#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>OrchidPal Setup</title>
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin />
  <link
    href="https://fonts.googleapis.com/css2?family=Fraunces:opsz,wght@9..144,600;9..144,700&family=Inter:wght@400;500;600;700&display=swap"
    rel="stylesheet" />
  <style>
    :root {
      /* Palette from app/globals.css (Botanical Luxury) */
      --brand-green: #3a5a40;
      --brand-green-deep: #14281d;
      --brand-purple: #9f5f80;
      --brand-purple-deep: #582c4d;

      /* Fonts */
      --font-sans: "Inter", "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      --font-serif: "Fraunces", ui-serif, Georgia, "Times New Roman", Times,
        serif;

      /* Theme mappings for this standalone setup page */
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
      padding: 35px 25px;
      border-radius: 20px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.1);
      text-align: center;
    }

    h1 {
      margin: 0 0 10px 0;
      color: var(--primary-color);
      font-weight: 700;
      font-size: 28px;
      font-family: var(--font-serif);
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }

    .title-orchid {
      color: var(--accent-color);
    }

    .subtitle {
      color: var(--text-light);
      margin-bottom: 30px;
      font-size: 15px;
    }

    #wifiList {
      text-align: left;
      margin-bottom: 30px;
      border: 2px solid #f0f2f5;
      border-radius: var(--border-radius);
      max-height: 220px;
      overflow-y: auto;
      background: #ffffff;
      scrollbar-width: thin;
      scrollbar-color: #cfd8dc #ffffff;
    }

    #wifiList::-webkit-scrollbar {
      width: 6px;
    }

    #wifiList::-webkit-scrollbar-track {
      background: #ffffff;
    }

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
    }

    form {
      text-align: left;
    }

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
      margin-bottom: 20px;
      border: 2px solid #e0e0e0;
      border-radius: var(--border-radius);
      box-sizing: border-box;
      font-size: 16px;
      transition:
        border-color 0.3s,
        box-shadow 0.3s;
      background-color: #fafafa;
    }

    input:focus {
      border-color: var(--primary-color);
      box-shadow: 0 0 0 4px rgba(88, 44, 77, 0.16);
      outline: none;
      background-color: #fff;
    }

    .password-wrapper {
      position: relative;
      margin-bottom: 20px;
    }

    .password-wrapper input {
      margin-bottom: 0;
      padding-right: 45px;
    }

    .toggle-password {
      position: absolute;
      top: 50%;
      right: 15px;
      transform: translateY(-50%);
      cursor: pointer;
      font-size: 18px;
      user-select: none;
      color: #888;
      opacity: 0.8;
      transition: opacity 0.2s;
    }

    .toggle-password:hover {
      opacity: 1;
    }

    button {
      width: 100%;
      padding: 16px;
      background: var(--primary-color);
      color: white;
      border: none;
      border-radius: var(--border-radius);
      font-size: 17px;
      font-weight: 700;
      cursor: pointer;
      box-shadow: 0 5px 15px rgba(88, 44, 77, 0.28);
      transition:
        transform 0.1s ease,
        box-shadow 0.1s ease,
        background-color 0.1s ease;
    }

    button:hover {
      background: var(--primary-dark);
      box-shadow: 0 10px 24px rgba(88, 44, 77, 0.34);
      transform: translateY(-2px);
    }

    button:active {
      background: var(--primary-dark);
      transform: translateY(1px);
      box-shadow: 0 2px 10px rgba(88, 44, 77, 0.26);
    }

    .btn-secondary {
      background: #f0f2f5;
      color: var(--text-dark);
      box-shadow: none;
      width: auto;
      padding: 10px 20px;
      font-size: 14px;
      margin-top: 15px;
      border: 1px solid #ddd;
    }

    .btn-secondary:hover {
      background: #e4e6e9;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
    }

    #status {
      margin-top: 20px;
      padding: 15px;
      border-radius: 10px;
      text-align: center;
      font-weight: 600;
      font-size: 15px;
      display: none;
      line-height: 1.5;
    }

    .status-loading {
      background-color: #fff3e0;
      color: #e67e22;
    }

    .status-error {
      background-color: #ffebee;
      color: #c62828;
    }

    .status-success {
      background-color: #f1f8e9;
      color: var(--accent-dark);
      border: 2px solid var(--accent-color);
      box-shadow: 0 5px 15px rgba(58, 90, 64, 0.18);
      animation: popIn 0.5s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    }

    @keyframes popIn {
      0% {
        transform: scale(0.8);
        opacity: 0;
      }

      100% {
        transform: scale(1);
        opacity: 1;
      }
    }

    input::-ms-reveal,
    input::-ms-clear {
      display: none;
    }

    input[type='password']::-webkit-textfield-decoration-container {
      visibility: hidden;
    }
  </style>
</head>

<body>
  <div class="container">
    <h1 class="title-orchid">OrchidPal Setup</h1>
    <p class="subtitle">Select your network to get started</p>

    <div id="wifiList"></div>

    <form id="setupForm" onsubmit="
          event.preventDefault();
          saveWiFi();
        ">
      <label for="ssid">Network Name (SSID)</label>
      <input type="text" id="ssid" placeholder="Tap a network above..." required readonly />

      <label for="pass">Password</label>
      <div class="password-wrapper">
        <input type="password" id="pass" placeholder="Enter WiFi Password" />
        <span class="toggle-password" onclick="togglePassword()">👁️</span>
      </div>

      <button type="submit" id="mainBtn">CONNECT DEVICE</button>
    </form>

    <div id="status"></div>
  </div>

  <style>
    @keyframes spin {
      to {
        transform: rotate(360deg);
      }
    }
  </style>

  <script>
    function selectWifi(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('pass').focus();
    }

    function togglePassword() {
      let passInput = document.getElementById('pass');
      let toggleBtn = document.querySelector('.toggle-password');

      if (passInput.type === 'password') {
        passInput.type = 'text';
        toggleBtn.innerText = '🙈';
      } else {
        passInput.type = 'password';
        toggleBtn.innerText = '👁️';
      }
    }

    function scanNetworks() {
      let list = document.getElementById('wifiList');
      list.innerHTML = `
                <div style="padding: 35px 25px; text-align:center; color: #999;">
                    <div style="display: inline-block; width: 24px; height: 24px; border: 3px solid rgba(88,44,77,0.25); border-radius: 50%; border-top-color: var(--primary-color); animation: spin 1s ease-in-out infinite; margin-bottom: 10px;"></div>
                    <br>Scanning networks...
                </div>
            `;

      fetch('/scan')
        .then((res) => res.json())
        .then((data) => {
          list.innerHTML = '';
          if (data.length === 0) {
            list.innerHTML = `
                            <div style="padding: 25px; text-align:center; color: #777;">
                                <p style="margin-top:0;">No networks found within range.</p>
                                <button type="button" class="btn-secondary" onclick="scanNetworks()">🔄 Scan Again</button>
                            </div>
                        `;
          } else {
            data.sort((a, b) => b.rssi - a.rssi);
            data.forEach((net) => {
              let rssiIcon = '📶';
              if (net.rssi >= -55) rssiIcon = '🟢 Excellent';
              else if (net.rssi >= -70) rssiIcon = '🟡 Good';
              else rssiIcon = '🟠 Weak';

              list.innerHTML += `
                            <div class="wifi-item" onclick="selectWifi('${net.ssid}')">
                                <span class="ssid-name">${net.ssid}</span>
                                <span class="rssi">${rssiIcon} (${net.rssi} dBm)</span>
                            </div>`;
            });
            list.innerHTML += `
                            <div style="text-align:center; padding: 10px; border-top: 1px solid #f0f2f5;">
                                <button type="button" class="btn-secondary" style="margin-top:5px; padding: 8px 15px;" onclick="scanNetworks()">🔄 Refresh List</button>
                            </div>
                        `;
          }
        })
        .catch(() => {
          list.innerHTML = `
                        <div style="padding: 25px; text-align:center; color:#c62828;">
                            <p style="margin-top:0;">Failed to scan WiFi networks.</p>
                            <button type="button" class="btn-secondary" onclick="scanNetworks()">🔄 Try Again</button>
                        </div>
                    `;
        });
    }

    scanNetworks();

    function saveWiFi() {
      let ssid = document.getElementById('ssid').value;
      let pass = document.getElementById('pass').value;
      let statusBtn = document.getElementById('status');
      let formBtn = document.getElementById('mainBtn');

      if (!ssid) {
        statusBtn.innerText = 'Please select a network first!';
        statusBtn.className = 'status-error';
        statusBtn.style.display = 'block';
        return;
      }

      statusBtn.innerHTML =
        'Verifying password...<br><small style="color:#e67e22;">Please do not close this page.</small>';
      statusBtn.className = 'status-loading';
      statusBtn.style.display = 'block';
      formBtn.innerText = 'CONNECTING...';
      formBtn.style.opacity = '0.7';
      formBtn.disabled = true;
      document.getElementById('setupForm').style.pointerEvents = 'none';

      fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`,
      })
        .then((response) => {
          if (!response.ok) throw new Error('Save failed');

          let checkInterval = setInterval(() => {
            fetch('/status')
              .then((res) => res.text())
              .then((state) => {
                if (state === 'success') {
                  clearInterval(checkInterval);

                  statusBtn.innerHTML = `
                                    <div style="font-size:20px; margin-bottom:5px;">✅ <b>Success!</b></div>
                                    <span style="color:#555;">Device connected to WiFi.</span><br><br>
                                    <div style="background-color:#ffffff; padding:12px; border-radius:8px; border:2px dashed var(--accent-color); color: var(--accent-dark); font-weight:bold; font-size:14px; box-shadow: 0 2px 5px rgba(0,0,0,0.05);">
                                        🚀 You can now close this page, reconnect to your WiFi network, and return to the app.
                                    </div>
                                `;
                  statusBtn.className = 'status-success';
                } else if (state === 'failed') {
                  clearInterval(checkInterval);
                  statusBtn.innerHTML =
                    '❌ <b>Wrong Password!</b><br>Please check and try again.';
                  statusBtn.className = 'status-error';

                  formBtn.innerText = 'CONNECT DEVICE';
                  formBtn.style.opacity = '1';
                  formBtn.disabled = false;
                  document.getElementById('setupForm').style.pointerEvents =
                    'auto';
                  document.getElementById('pass').value = '';
                  document.getElementById('pass').focus();
                }
              })
              .catch((err) => { });
          }, 1500);
        })
        .catch(() => {
          statusBtn.innerText = 'Error sending credentials to device.';
          statusBtn.className = 'status-error';
          formBtn.innerText = 'CONNECT DEVICE';
          formBtn.style.opacity = '1';
          formBtn.disabled = false;
          document.getElementById('setupForm').style.pointerEvents = 'auto';
        });
    }
  </script>
</body>

</html>
)rawliteral";

#endif
