#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>GrowPal Setup</title>
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin />
  <link
    href="https://fonts.googleapis.com/css2?family=Fraunces:opsz,wght@9..144,600;9..144,700&family=Inter:wght@400;500;600;700&display=swap"
    rel="stylesheet" />
  <style>
    :root {
      --brand-green: #3a5a40;
      --brand-green-deep: #14281d;
      --brand-purple: #9f5f80;
      --brand-purple-deep: #582c4d;
      --font-sans: "Inter", "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      --font-serif: "Fraunces", ui-serif, Georgia, "Times New Roman", Times, serif;
      --primary-color: var(--brand-purple);
      --primary-dark: var(--brand-purple-deep);
      --accent-color: var(--brand-green);
      --accent-dark: var(--brand-green-deep);
      --bg-gradient: linear-gradient(135deg, #fdfcf8 0%, #f4e9f0 45%, #e7efe9 100%);
      --text-dark: #333333;
      --text-light: #666666;
      --border-radius: 12px;
    }

    body {
      font-family: var(--font-sans);
      margin: 0;
      padding: 20px;
      background: var(--bg-gradient);
      color: var(--text-dark);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      box-sizing: border-box;
    }

    .container {
      background-color: #ffffff;
      width: 100%;
      max-width: 420px;
      padding: 28px 22px 30px;
      border-radius: 20px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.1);
      text-align: center;
    }

    h1 {
      margin: 0 0 6px 0;
      color: var(--primary-color);
      font-weight: 700;
      font-size: 26px;
      font-family: var(--font-serif);
    }

    .title-brand { color: var(--accent-color); }

    .subtitle {
      color: var(--text-light);
      margin: 0 0 20px;
      font-size: 14px;
    }

    /* Lucide-style icons */
    .icon {
      width: 18px;
      height: 18px;
      stroke: currentColor;
      stroke-width: 2;
      stroke-linecap: round;
      stroke-linejoin: round;
      fill: none;
      flex-shrink: 0;
      display: inline-block;
      vertical-align: middle;
    }

    .icon-sm { width: 16px; height: 16px; }

    .icon-btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }

    /* Tabs */
    .tabs {
      display: flex;
      gap: 6px;
      padding: 5px;
      margin-bottom: 20px;
      background: #f4e9f0;
      border-radius: 14px;
    }

    .tab {
      flex: 1;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 11px 10px;
      border: none;
      border-radius: 10px;
      background: transparent;
      color: var(--text-light);
      font-size: 14px;
      font-weight: 600;
      font-family: var(--font-sans);
      cursor: pointer;
      box-shadow: none;
      transform: none;
      transition: background 0.2s, color 0.2s, box-shadow 0.2s;
    }

    .tab:hover {
      background: rgba(255, 255, 255, 0.55);
      color: var(--text-dark);
      transform: none;
      box-shadow: none;
    }

    .tab.active {
      background: #ffffff;
      color: var(--primary-dark);
      box-shadow: 0 2px 8px rgba(88, 44, 77, 0.12);
    }

    .tab-panel { display: none; text-align: left; }
    .tab-panel.active { display: block; }

    #wifiList {
      text-align: left;
      margin-bottom: 16px;
      border: 2px solid #f0f2f5;
      border-radius: var(--border-radius);
      max-height: 200px;
      overflow-y: auto;
      background: #ffffff;
      scrollbar-width: thin;
      scrollbar-color: #cfd8dc #ffffff;
    }

    #wifiList::-webkit-scrollbar { width: 6px; }
    #wifiList::-webkit-scrollbar-track { background: #ffffff; }
    #wifiList::-webkit-scrollbar-thumb {
      background-color: #cfd8dc;
      border-radius: 10px;
    }

    .wifi-item {
      padding: 14px 18px;
      border-bottom: 1px solid #f0f2f5;
      cursor: pointer;
      display: flex;
      justify-content: space-between;
      align-items: center;
      transition: all 0.2s ease;
    }

    .wifi-item:hover {
      background-color: #f4e9f0;
      padding-left: 24px;
    }

    .ssid-name {
      font-weight: 600;
      color: var(--text-dark);
      font-size: 15px;
    }

    .rssi {
      color: #9e9e9e;
      font-size: 12px;
      font-weight: 500;
      display: inline-flex;
      align-items: center;
      gap: 8px;
    }

    .signal-dot {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      flex-shrink: 0;
      box-shadow: 0 0 0 2px rgba(0, 0, 0, 0.06);
    }

    .signal-dot.strong { background: #22c55e; }
    .signal-dot.weak   { background: #ef4444; }

    label {
      display: block;
      margin-bottom: 8px;
      font-weight: 600;
      color: var(--text-dark);
      font-size: 14px;
    }

    input {
      width: 100%;
      padding: 14px 16px;
      margin-bottom: 16px;
      border: 2px solid #e0e0e0;
      border-radius: var(--border-radius);
      box-sizing: border-box;
      font-size: 16px;
      transition: border-color 0.3s, box-shadow 0.3s;
      background-color: #fafafa;
      font-family: var(--font-sans);
    }

    input:focus {
      border-color: var(--primary-color);
      box-shadow: 0 0 0 4px rgba(88, 44, 77, 0.16);
      outline: none;
      background-color: #fff;
    }

    .password-wrapper {
      position: relative;
      margin-bottom: 16px;
    }

    .password-wrapper input {
      margin-bottom: 0;
      padding-right: 46px;
    }

    .toggle-password {
      position: absolute;
      top: 50%;
      right: 12px;
      transform: translateY(-50%);
      cursor: pointer;
      color: #888;
      opacity: 0.85;
      display: inline-flex;
      padding: 4px;
      border: none;
      background: transparent;
      box-shadow: none;
      width: auto;
    }

    .toggle-password:hover {
      opacity: 1;
      background: transparent;
      transform: translateY(-50%);
      box-shadow: none;
    }

    .field-row {
      display: flex;
      gap: 12px;
    }

    .field-row .field-grow { flex: 1; min-width: 0; }
    .field-row .field-port { width: 96px; flex-shrink: 0; }
    .field-port input { text-align: center; }

    .panel-hint {
      margin: 0 0 14px;
      font-size: 12px;
      color: var(--text-light);
      line-height: 1.45;
      display: flex;
      align-items: flex-start;
      gap: 8px;
    }

    .panel-hint .icon { margin-top: 1px; color: var(--accent-color); }

    .mqtt-card {
      background: linear-gradient(145deg, #faf8f5 0%, #f4e9f0 55%, #eef5ef 100%);
      border: 1px solid #eadfe6;
      border-radius: var(--border-radius);
      padding: 16px 14px 4px;
      margin-bottom: 16px;
    }

    .mqtt-card input { background-color: #ffffff; }

    button.primary {
      width: 100%;
      padding: 15px;
      background: var(--primary-color);
      color: white;
      border: none;
      border-radius: var(--border-radius);
      font-size: 16px;
      font-weight: 700;
      cursor: pointer;
      box-shadow: 0 5px 15px rgba(88, 44, 77, 0.28);
      font-family: var(--font-sans);
      transition: transform 0.1s ease, box-shadow 0.1s ease, background-color 0.1s ease;
    }

    button.primary:hover {
      background: var(--primary-dark);
      box-shadow: 0 10px 24px rgba(88, 44, 77, 0.34);
      transform: translateY(-2px);
    }

    button.primary:active {
      transform: translateY(1px);
      box-shadow: 0 2px 10px rgba(88, 44, 77, 0.26);
    }

    button.primary:disabled {
      opacity: 0.7;
      cursor: not-allowed;
      transform: none;
    }

    button.primary.mqtt-save {
      background: var(--accent-color);
      box-shadow: 0 5px 15px rgba(58, 90, 64, 0.28);
    }

    button.primary.mqtt-save:hover {
      background: var(--accent-dark);
      box-shadow: 0 10px 24px rgba(58, 90, 64, 0.34);
    }

    .btn-secondary {
      background: #f0f2f5;
      color: var(--text-dark);
      box-shadow: none;
      width: auto;
      padding: 10px 16px;
      font-size: 13px;
      margin-top: 10px;
      border: 1px solid #ddd;
      border-radius: 10px;
      font-weight: 600;
      cursor: pointer;
      font-family: var(--font-sans);
      display: inline-flex;
      align-items: center;
      gap: 6px;
    }

    .btn-secondary:hover {
      background: #e4e6e9;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
      transform: none;
    }

    /* Nút Refresh cố định ngoài list — luôn dễ bấm */
    .scan-bar {
      margin: 0 0 16px;
    }

    .btn-scan {
      width: 100%;
      justify-content: center;
      margin-top: 0;
      padding: 12px 16px;
      font-size: 14px;
      background: #fff;
      border: 2px solid #eadfe6;
      color: var(--primary-dark);
      border-radius: var(--border-radius);
    }

    .btn-scan:hover {
      background: #f4e9f0;
      border-color: var(--primary-color);
    }

    .btn-scan:disabled {
      opacity: 0.65;
      cursor: wait;
    }

    .list-empty {
      padding: 28px 16px;
      text-align: center;
      color: #777;
      font-size: 14px;
    }

    .list-empty.error { color: #c62828; }

    #status {
      margin-top: 18px;
      padding: 14px;
      border-radius: 10px;
      text-align: center;
      font-weight: 600;
      font-size: 14px;
      display: none;
      line-height: 1.5;
    }

    .status-loading { background-color: #fff3e0; color: #e67e22; }
    .status-error { background-color: #ffebee; color: #c62828; }
    .status-success {
      background-color: #f1f8e9;
      color: var(--accent-dark);
      border: 2px solid var(--accent-color);
      box-shadow: 0 5px 15px rgba(58, 90, 64, 0.18);
      animation: popIn 0.5s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    }

    @keyframes popIn {
      0% { transform: scale(0.8); opacity: 0; }
      100% { transform: scale(1); opacity: 1; }
    }

    @keyframes spin {
      to { transform: rotate(360deg); }
    }

    .spin { animation: spin 1s linear infinite; }

    input::-ms-reveal,
    input::-ms-clear { display: none; }

    input[type='password']::-webkit-textfield-decoration-container {
      visibility: hidden;
    }
  </style>
</head>

<body>
  <div class="container">
    <h1 class="title-brand">GrowPal Setup</h1>
    <p class="subtitle">Configure WiFi and MQTT independently</p>

    <div class="tabs" role="tablist">
      <button type="button" class="tab active" id="tab-wifi" role="tab" aria-selected="true" onclick="switchTab('wifi')">
        <svg class="icon" viewBox="0 0 24 24" aria-hidden="true"><path d="M12 20h.01"/><path d="M2 8.82a15 15 0 0 1 20 0"/><path d="M5 12.859a10 10 0 0 1 14 0"/><path d="M8.5 16.429a5 5 0 0 1 7 0"/></svg>
        WiFi
      </button>
      <button type="button" class="tab" id="tab-mqtt" role="tab" aria-selected="false" onclick="switchTab('mqtt')">
        <svg class="icon" viewBox="0 0 24 24" aria-hidden="true"><path d="M4.9 19.1C1 15.2 1 8.8 4.9 4.9"/><path d="M7.8 16.2c-2.3-2.3-2.3-6.1 0-8.5"/><circle cx="12" cy="12" r="2"/><path d="M16.2 7.8c2.3 2.3 2.3 6.1 0 8.5"/><path d="M19.1 4.9C23 8.8 23 15.1 19.1 19"/></svg>
        MQTT
      </button>
    </div>

    <!-- ===== TAB WIFI ===== -->
    <div id="panel-wifi" class="tab-panel active" role="tabpanel">
      <div id="wifiList"></div>

      <div class="scan-bar">
        <button type="button" class="btn-secondary btn-scan icon-btn" id="scanBtn" onclick="scanNetworks()">
          <svg class="icon icon-sm" viewBox="0 0 24 24"><path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M8 16H3v5"/></svg>
          <span id="scanBtnLabel">Refresh networks</span>
        </button>
      </div>

      <label for="ssid">Network Name (SSID)</label>
      <input type="text" id="ssid" placeholder="Tap a network above..." readonly />

      <label for="pass">Password</label>
      <div class="password-wrapper">
        <input type="password" id="pass" placeholder="Enter WiFi Password" />
        <button type="button" class="toggle-password" onclick="togglePassword('pass', this)" aria-label="Toggle password">
          <svg class="icon icon-eye" viewBox="0 0 24 24"><path d="M2.062 12.348a1 1 0 0 1 0-.696 10.75 10.75 0 0 1 19.876 0 1 1 0 0 1 0 .696 10.75 10.75 0 0 1-19.876 0"/><circle cx="12" cy="12" r="3"/></svg>
        </button>
      </div>

      <button type="button" class="primary icon-btn" id="wifiBtn" onclick="saveWiFi()">
        <svg class="icon" viewBox="0 0 24 24"><path d="M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"/><path d="M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7"/><path d="M7 3v4a1 1 0 0 0 1 1h7"/></svg>
        Save WiFi
      </button>
    </div>

    <!-- ===== TAB MQTT ===== -->
    <div id="panel-mqtt" class="tab-panel" role="tabpanel">
      <p class="panel-hint">
        <svg class="icon icon-sm" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><path d="M12 16v-4"/><path d="M12 8h.01"/></svg>
        Optional. Leave unchanged to keep factory defaults. Only saved when you press Save MQTT.
      </p>

      <div class="mqtt-card">
        <div class="field-row">
          <div class="field-grow">
            <label for="mqtt_server">Broker IP / Domain</label>
            <input type="text" id="mqtt_server" placeholder="e.g. 192.168.1.10" autocomplete="off" />
          </div>
          <div class="field-port">
            <label for="mqtt_port">Port</label>
            <input type="number" id="mqtt_port" value="1883" min="1" max="65535" />
          </div>
        </div>

        <label for="mqtt_user">Username</label>
        <input type="text" id="mqtt_user" placeholder="Optional" autocomplete="off" />

        <label for="mqtt_pass">Password</label>
        <div class="password-wrapper">
          <input type="password" id="mqtt_pass" placeholder="Optional" autocomplete="new-password" />
          <button type="button" class="toggle-password" onclick="togglePassword('mqtt_pass', this)" aria-label="Toggle password">
            <svg class="icon icon-eye" viewBox="0 0 24 24"><path d="M2.062 12.348a1 1 0 0 1 0-.696 10.75 10.75 0 0 1 19.876 0 1 1 0 0 1 0 .696 10.75 10.75 0 0 1-19.876 0"/><circle cx="12" cy="12" r="3"/></svg>
          </button>
        </div>
      </div>

      <button type="button" class="primary mqtt-save icon-btn" id="mqttBtn" onclick="saveMqtt()">
        <svg class="icon" viewBox="0 0 24 24"><path d="M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"/><path d="M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7"/><path d="M7 3v4a1 1 0 0 0 1 1h7"/></svg>
        Save MQTT
      </button>
    </div>

    <div id="status"></div>
  </div>

  <script>
    var ICON_EYE = '<svg class="icon icon-eye" viewBox="0 0 24 24"><path d="M2.062 12.348a1 1 0 0 1 0-.696 10.75 10.75 0 0 1 19.876 0 1 1 0 0 1 0 .696 10.75 10.75 0 0 1-19.876 0"/><circle cx="12" cy="12" r="3"/></svg>';
    var ICON_EYE_OFF = '<svg class="icon icon-eye" viewBox="0 0 24 24"><path d="M10.733 5.076a10.744 10.744 0 0 1 11.205 6.575 1 1 0 0 1 0 .696 10.747 10.747 0 0 1-1.444 2.49"/><path d="M14.084 14.158a3 3 0 0 1-4.242-4.242"/><path d="M17.479 17.499a10.75 10.75 0 0 1-15.417-5.151 1 1 0 0 1 0-.696 10.75 10.75 0 0 1 4.446-5.143"/><path d="m2 2 20 20"/></svg>';
    var ICON_REFRESH = '<svg class="icon icon-sm" viewBox="0 0 24 24"><path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M8 16H3v5"/></svg>';

    function switchTab(name) {
      var isWifi = name === 'wifi';
      document.getElementById('tab-wifi').classList.toggle('active', isWifi);
      document.getElementById('tab-mqtt').classList.toggle('active', !isWifi);
      document.getElementById('tab-wifi').setAttribute('aria-selected', isWifi ? 'true' : 'false');
      document.getElementById('tab-mqtt').setAttribute('aria-selected', isWifi ? 'false' : 'true');
      document.getElementById('panel-wifi').classList.toggle('active', isWifi);
      document.getElementById('panel-mqtt').classList.toggle('active', !isWifi);
      document.getElementById('status').style.display = 'none';
    }

    function selectWifi(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('pass').focus();
    }

    function togglePassword(inputId, btn) {
      var input = document.getElementById(inputId);
      if (!input) return;
      if (input.type === 'password') {
        input.type = 'text';
        btn.innerHTML = ICON_EYE_OFF;
      } else {
        input.type = 'password';
        btn.innerHTML = ICON_EYE;
      }
    }

    function showStatus(html, cls) {
      var el = document.getElementById('status');
      el.innerHTML = html;
      el.className = cls;
      el.style.display = 'block';
    }

    function loadSavedConfig() {
      fetch('/config')
        .then(function (res) { return res.json(); })
        .then(function (cfg) {
          if (cfg.mqtt_server) document.getElementById('mqtt_server').value = cfg.mqtt_server;
          if (cfg.mqtt_port) document.getElementById('mqtt_port').value = cfg.mqtt_port;
          if (cfg.mqtt_user !== undefined) document.getElementById('mqtt_user').value = cfg.mqtt_user;
          if (cfg.mqtt_pass !== undefined) document.getElementById('mqtt_pass').value = cfg.mqtt_pass;
        })
        .catch(function () {});
    }

    function setScanBusy(busy) {
      var btn = document.getElementById('scanBtn');
      var label = document.getElementById('scanBtnLabel');
      btn.disabled = busy;
      label.textContent = busy ? 'Scanning...' : 'Refresh networks';
    }

    function scanNetworks() {
      var list = document.getElementById('wifiList');
      setScanBusy(true);
      list.innerHTML =
        '<div class="list-empty">' +
        '<svg class="icon spin" style="width:24px;height:24px;color:var(--primary-color);margin-bottom:8px;" viewBox="0 0 24 24"><path d="M21 12a9 9 0 1 1-6.219-8.56"/></svg>' +
        '<br>Scanning networks...</div>';

      fetch('/scan')
        .then(function (res) { return res.json(); })
        .then(function (data) {
          list.innerHTML = '';
          if (!data.length) {
            list.innerHTML = '<div class="list-empty">No networks found. Tap Refresh below.</div>';
            setScanBusy(false);
            return;
          }
          data.sort(function (a, b) { return b.rssi - a.rssi; });
          data.forEach(function (net) {
            var safeSsid = String(net.ssid).replace(/\\/g, '\\\\').replace(/'/g, "\\'");
            var strong = net.rssi >= -70;
            var dotClass = strong ? 'strong' : 'weak';
            var tip = strong ? 'Strong signal' : 'Weak signal';
            list.innerHTML +=
              '<div class="wifi-item" onclick="selectWifi(\'' + safeSsid + '\')">' +
              '<span class="ssid-name">' + net.ssid + '</span>' +
              '<span class="rssi" title="' + tip + '">' +
              '<span class="signal-dot ' + dotClass + '"></span>' +
              net.rssi + ' dBm</span></div>';
          });
          setScanBusy(false);
        })
        .catch(function () {
          list.innerHTML = '<div class="list-empty error">Scan failed. Tap Refresh below to try again.</div>';
          setScanBusy(false);
        });
    }

    function resetWifiBtn() {
      var btn = document.getElementById('wifiBtn');
      btn.disabled = false;
      btn.style.opacity = '1';
    }

    function saveWiFi() {
      var ssid = document.getElementById('ssid').value;
      var pass = document.getElementById('pass').value;
      var btn = document.getElementById('wifiBtn');

      if (!ssid) {
        showStatus('Please select a network first!', 'status-error');
        return;
      }

      showStatus('Connecting & saving WiFi...<br><small style="color:#e67e22;">Please do not close this page.</small>', 'status-loading');
      btn.disabled = true;
      btn.style.opacity = '0.7';

      fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass)
      })
        .then(function (response) {
          if (!response.ok) throw new Error('Save failed');
          var checkInterval = setInterval(function () {
            fetch('/status')
              .then(function (res) { return res.text(); })
              .then(function (state) {
                if (state === 'success') {
                  clearInterval(checkInterval);
                  showStatus(
                    '<div style="font-size:18px;margin-bottom:5px;">WiFi Saved</div>' +
                    '<span style="color:#555;">Connected and credentials stored in flash.</span><br><br>' +
                    '<div style="background:#fff;padding:12px;border-radius:8px;border:2px dashed var(--accent-color);color:var(--accent-dark);font-weight:bold;font-size:13px;">' +
                    'You can close this page, reconnect to your WiFi, and return to the app.</div>',
                    'status-success'
                  );
                } else if (state === 'save_error') {
                  clearInterval(checkInterval);
                  showStatus('Connected but flash save failed. Please try Save WiFi again.', 'status-error');
                  resetWifiBtn();
                } else if (state === 'failed') {
                  clearInterval(checkInterval);
                  showStatus('Wrong password or connection failed. Please retry.', 'status-error');
                  resetWifiBtn();
                  document.getElementById('pass').value = '';
                  document.getElementById('pass').focus();
                }
              })
              .catch(function () {});
          }, 1500);
        })
        .catch(function () {
          showStatus('Error sending WiFi credentials to device.', 'status-error');
          resetWifiBtn();
        });
    }

    function saveMqtt() {
      var mqttServer = document.getElementById('mqtt_server').value.trim();
      var mqttPort = document.getElementById('mqtt_port').value.trim() || '1883';
      var mqttUser = document.getElementById('mqtt_user').value.trim();
      var mqttPass = document.getElementById('mqtt_pass').value;
      var btn = document.getElementById('mqttBtn');

      if (!mqttServer) {
        showStatus('Please enter the MQTT broker address.', 'status-error');
        document.getElementById('mqtt_server').focus();
        return;
      }

      var portNum = parseInt(mqttPort, 10);
      if (isNaN(portNum) || portNum < 1 || portNum > 65535) {
        showStatus('MQTT port must be between 1 and 65535.', 'status-error');
        document.getElementById('mqtt_port').focus();
        return;
      }

      showStatus('Saving MQTT to flash...', 'status-loading');
      btn.disabled = true;
      btn.style.opacity = '0.7';

      var body =
        'mqtt_server=' + encodeURIComponent(mqttServer) +
        '&mqtt_port=' + encodeURIComponent(String(portNum)) +
        '&mqtt_user=' + encodeURIComponent(mqttUser) +
        '&mqtt_pass=' + encodeURIComponent(mqttPass);

      fetch('/save-mqtt', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body
      })
        .then(function (response) {
          return response.text().then(function (text) {
            if (!response.ok) throw new Error(text || 'Save failed');
            return text;
          });
        })
        .then(function () {
          showStatus(
            '<div style="font-size:18px;margin-bottom:5px;">MQTT Saved</div>' +
            '<span style="color:#555;">Verified in flash. Device is reconnecting to the broker now.</span>',
            'status-success'
          );
          btn.disabled = false;
          btn.style.opacity = '1';
        })
        .catch(function (err) {
          var msg = (err && err.message === 'flash_error')
            ? 'Flash write failed. Please try Save MQTT again.'
            : 'Error saving MQTT settings.';
          showStatus(msg, 'status-error');
          btn.disabled = false;
          btn.style.opacity = '1';
        });
    }

    scanNetworks();
    loadSavedConfig();
  </script>
</body>

</html>
)rawliteral";

#endif
