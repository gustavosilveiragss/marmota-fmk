#include "BtSource.h"

#ifdef MRM_HAS_BT_SOURCE

#include <Arduino.h>
#include <cstring>

#include "Log.h"

namespace mrm {

namespace {

constexpr uint32_t kScanGuardMs = 16000; // varredura de 12,8 s; passou disso a pilha nao respondeu
constexpr uint32_t kConnectMs = 12000;   // page timeout do controlador mais folga; abaixo dos 15 s do Navigator
constexpr uint32_t kPageWaitMs = 6000;   // page timeout padrao do controlador (5,12 s) mais folga

int32_t silence(uint8_t* data, int32_t len) {
    if (!data || len <= 0)
        return 0; // a pilha chama (NULL, -1) ao esvaziar a fila quando o stream para
    memset(data, 0, len);
    return len;
}

bool busy(BtSource::State s) {
    return s == BtSource::State::Connecting || s == BtSource::State::Connected || s == BtSource::State::Streaming;
}

} // namespace

void BtSource::start() {
    if (state_ != State::Off)
        return;
    set_auto_reconnect(false);
    set_on_connection_state_changed(onConnection, this);
    set_on_audio_state_changed(onAudio, this);
    if (!get_data_cb && !get_data_in_frames_cb)
        set_data_callback(silence);

    foundCount_ = 0;
    head_ = tail_ = 0;
    ready_ = scanLive_ = connectPending_ = rescan_ = dropExpected_ = paging_ = false;
    state_ = State::Scanning;
    stateAt_ = millis();
    BluetoothA2DPSource::start();
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        log("BtSource: pilha nao subiu");
        stop();
    }
}

void BtSource::stop() {
    if (state_ == State::Off)
        return;
    disconnect();
    // A pilha ignora o disconnect durante o page, e o deinit do A2DP no meio dele libera btc_av_cb;
    // o fim do page chega depois na task BTC e derruba o chip (LoadProhibited em btc_a2dp_cb_handler).
    const uint32_t t0 = millis();
    while (paging_ && millis() - t0 < kPageWaitMs)
        delay(10);
    end(false);
    // end(true) tambem faz mem_release, que impede religar; aqui so o que desfaz o init de start().
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    btStop();
    state_ = State::Off;
    ready_ = scanLive_ = connectPending_ = rescan_ = dropExpected_ = paging_ = false;
    foundCount_ = 0;
}

void BtSource::update() {
    const State s = state_;
    if (s == State::Off)
        return;
    uint8_t t = tail_;
    while (t != head_.load(std::memory_order_acquire)) {
        addFound(queue_[t]);
        t = (t + 1) % kQueue;
        tail_.store(t, std::memory_order_release);
    }
    const uint32_t now = millis();
    if (s == State::Scanning && now - stateAt_ > kScanGuardMs) {
        scanLive_ = false;
        state_ = ready_ ? State::Idle : State::Failed;
    } else if (s == State::Connecting && now - stateAt_ > kConnectMs) {
        connectPending_ = false;
        if (ready_)
            BluetoothA2DPSource::disconnect();
        state_ = State::Failed;
    }
}

bool BtSource::scan() {
    const State s = state_;
    if (s == State::Off || busy(s))
        return false;
    foundCount_ = 0;
    tail_ = head_.load();
    state_ = State::Scanning;
    stateAt_ = millis();
    if (!ready_)
        return true; // a varredura do start() ainda vai chegar
    if (scanLive_) {
        rescan_ = true;
        esp_bt_gap_cancel_discovery(); // o STOPPED relanca
        return true;
    }
    return beginDiscovery();
}

bool BtSource::connect(const uint8_t bda[6]) {
    const State s = state_;
    if (s == State::Off || busy(s))
        return false;
    memcpy(peer_bd_addr, bda, 6);
    memcpy(last_connection, bda, 6);
    is_target_status_active = true;
    state_ = State::Connecting;
    stateAt_ = millis();
    connectPending_ = true;
    if (scanLive_)
        esp_bt_gap_cancel_discovery(); // o STOPPED consome o pendente
    else if (ready_ && connectPending_.exchange(false))
        connectNow();
    return true;
}

void BtSource::disconnect() {
    const State s = state_;
    if (!busy(s))
        return;
    connectPending_ = false;
    if (s != State::Connecting)
        dropExpected_ = true;
    if (ready_)
        BluetoothA2DPSource::disconnect();
    state_ = State::Idle;
}

void BtSource::connectNow() {
    paging_ = true;
    if (connect_to(peer_bd_addr))
        return;
    paging_ = false;
    State e = State::Connecting;
    state_.compare_exchange_strong(e, State::Failed);
}

bool BtSource::beginDiscovery() {
    if (esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0) == ESP_OK)
        return true;
    State e = State::Scanning;
    state_.compare_exchange_strong(e, State::Idle);
    return false;
}

void BtSource::a2d_app_heart_beat(void* arg) {
    const State s = state_;
    if (s == State::Connected || s == State::Streaming)
        BluetoothA2DPSource::a2d_app_heart_beat(arg);
}

void BtSource::app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT:
        onFound(param->disc_res);
        break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        onDiscovery(param->disc_st_chg.state);
        break;
    default:
        BluetoothA2DPSource::app_gap_callback(event, param); // PIN e confirmacao de pareamento
    }
}

void BtSource::onDiscovery(esp_bt_gap_discovery_state_t st) {
    if (st == ESP_BT_GAP_DISCOVERY_STARTED) {
        ready_ = true;
        scanLive_ = true;
        if (connectPending_)
            esp_bt_gap_cancel_discovery();
        return;
    }
    scanLive_ = false;
    if (connectPending_.exchange(false))
        connectNow();
    else if (rescan_.exchange(false))
        beginDiscovery();
    else {
        State e = State::Scanning;
        state_.compare_exchange_strong(e, State::Idle);
    }
}

void BtSource::onFound(const esp_bt_gap_cb_param_t::disc_res_param& r) {
    uint32_t cod = 0;
    int rssi = -128;
    const uint8_t* eir = nullptr;
    const char* bdname = nullptr;
    int bdnameLen = 0;
    for (int i = 0; i < r.num_prop; ++i) {
        const esp_bt_gap_dev_prop_t& p = r.prop[i];
        switch (p.type) {
        case ESP_BT_GAP_DEV_PROP_COD:
            cod = *static_cast<uint32_t*>(p.val);
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = *static_cast<int8_t*>(p.val);
            break;
        case ESP_BT_GAP_DEV_PROP_EIR:
            eir = static_cast<uint8_t*>(p.val);
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
            bdname = static_cast<const char*>(p.val);
            bdnameLen = p.len;
            break;
        default:
            break;
        }
    }
#ifdef MRM_BT_TRACE // todo resultado cru da varredura, para ver por que um fone nao entra na lista
    Serial.printf("bt-trace %02x:%02x:%02x:%02x:%02x:%02x cod=0x%06x rssi=%d eir=%d name=%.*s\n", r.bda[0], r.bda[1],
                  r.bda[2], r.bda[3], r.bda[4], r.bda[5], static_cast<unsigned>(cod), rssi, eir != nullptr,
                  bdnameLen, bdname ? bdname : "");
#endif
    if (!esp_bt_gap_is_valid_cod(cod) || !is_valid_cod_service(cod))
        return;
    const uint8_t* name = nullptr;
    uint8_t len = 0;
    if (eir) {
        name = esp_bt_gap_resolve_eir_data(const_cast<uint8_t*>(eir), ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
        if (!name)
            name = esp_bt_gap_resolve_eir_data(const_cast<uint8_t*>(eir), ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
    }
    if (!name && bdname) {
        name = reinterpret_cast<const uint8_t*>(bdname);
        len = bdnameLen > 0 ? bdnameLen : 0;
    }
    if (!name || len == 0)
        return; // sem nome nao ha o que mostrar na lista
    Event e;
    memcpy(e.bda, r.bda, 6);
    e.rssi = static_cast<int8_t>(rssi);
    len = len > kNameLen ? kNameLen : len;
    memcpy(e.name, name, len);
    e.name[len] = '\0';
    push(e);
}

void BtSource::push(const Event& e) {
    const uint8_t h = head_.load(std::memory_order_relaxed);
    const uint8_t n = (h + 1) % kQueue;
    if (n == tail_.load(std::memory_order_acquire)) {
        ++dropped_;
        return;
    }
    queue_[h] = e;
    head_.store(n, std::memory_order_release);
}

void BtSource::addFound(const Event& e) {
    for (uint8_t i = 0; i < foundCount_; ++i) {
        if (memcmp(found_[i].bda, e.bda, 6) != 0)
            continue;
        found_[i].rssi = e.rssi;
        return;
    }
    if (foundCount_ >= kMaxFound)
        return;
    Found& f = found_[foundCount_++];
    memcpy(f.bda, e.bda, 6);
    memcpy(f.name, e.name, sizeof(f.name));
    f.rssi = e.rssi;
}

void BtSource::onConnection(esp_a2d_connection_state_t st, void* self) {
    BtSource& b = *static_cast<BtSource*>(self);
    if (st == ESP_A2D_CONNECTION_STATE_CONNECTED || st == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        b.paging_ = false;
    if (st == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        State e = State::Connecting;
        if (b.state_.compare_exchange_strong(e, State::Connected))
            b.a2d_app_heart_beat(nullptr); // sem isto a midia so comeca no proximo tick de 10 s
        else if (e == State::Idle || e == State::Failed)
            b.BluetoothA2DPSource::disconnect(); // conectou depois de cancelarmos
        return;
    }
    if (st != ESP_A2D_CONNECTION_STATE_DISCONNECTED || b.dropExpected_.exchange(false))
        return;
    State e = State::Connecting;
    if (b.state_.compare_exchange_strong(e, State::Failed))
        return;
    for (State from : {State::Connected, State::Streaming}) {
        e = from;
        if (b.state_.compare_exchange_strong(e, State::Idle))
            return;
    }
}

void BtSource::onAudio(esp_a2d_audio_state_t st, void* self) {
    BtSource& b = *static_cast<BtSource*>(self);
    const bool started = st == ESP_A2D_AUDIO_STATE_STARTED;
    State e = started ? State::Connected : State::Streaming;
    b.state_.compare_exchange_strong(e, started ? State::Streaming : State::Connected);
}

} // namespace mrm

#endif
