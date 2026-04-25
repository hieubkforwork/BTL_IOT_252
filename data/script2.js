// Section navigation
function showSection(sectionId) {
    // Hide all sections
    const sections = document.querySelectorAll('.section');
    sections.forEach(section => {
        section.classList.remove('active');
    });

    // Show selected section
    const targetSection = document.getElementById(sectionId);
    if (targetSection) {
        targetSection.classList.add('active');
    }
}

// File upload display
document.addEventListener('DOMContentLoaded', function () {
    const firmwareFile = document.getElementById('firmware-file');
    if (firmwareFile) {
        firmwareFile.addEventListener('change', function (e) {
            const fileName = e.target.files[0]?.name;
            if (fileName) {
                const uploadDiv = document.querySelector('.file-upload');
                uploadDiv.innerHTML = `
                    <div style="font-size: 40px; margin-bottom: 10px;">✅</div>
                    <div style="font-weight: 600; margin-bottom: 5px;">${fileName}</div>
                    <div style="font-size: 12px; opacity: 0.8;">Click to change file</div>
                `;
            }
        });
    }

    // Simulate real-time updates (optional - can be connected to ESP32)
    setInterval(() => {
        // Update random temperature
        const tempValue = document.querySelector('.info-card .info-row:nth-child(1) .info-value');
        if (tempValue && tempValue.textContent.includes('°C')) {
            const temp = (25 + Math.random() * 2).toFixed(1);
            tempValue.textContent = temp + '°C';
        }
    }, 5000);
});

async function connectWiFi() {
    const btn = document.getElementById("btn-connect-wifi");
    if (btn) {
        btn.innerHTML = "<span> Connecting...</span>";
        btn.disabled = true;
    }

    try {
        await fetch("/api/wifi/connect", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                ssid: document.getElementById("ssid").value,
                password: document.getElementById("password").value
            })
        });

        pollWiFiStatus();
    } catch (error) {
        console.error("Error:", error);
        if (btn) {
            btn.innerHTML = "<span> Error</span>";
            setTimeout(() => {
                btn.innerHTML = "<span>Connect</span>";
                btn.disabled = false;
            }, 2000);
        }
    }
}

function pollWiFiStatus() {
    let retry = 0;
    const maxRetry = 5; // ~20s

    const btn = document.getElementById("btn-connect-wifi");

    const interval = setInterval(async () => {
        retry++;

        try {
            const res = await fetch("/api/wifi/status");
            const data = await res.json();

            // ✅ CONNECT SUCCESS
            if (data.connected) {
                clearInterval(interval);

                updateWiFiUI();

                if (btn) {
                    btn.innerHTML = "<span> Connect</span>";
                    btn.disabled = false;
                }

                return;
            }

            // TIMEOUT → FAIL
            if (retry >= maxRetry) {
                clearInterval(interval);

                console.warn("WiFi connect timeout");

                if (btn) {
                    btn.innerHTML = `<span>Connect</span>`;
                    btn.disabled = false;
                }

                // reset UI
                updateWiFiUI();
                return;
            }
            if(btn) btn.innerHTML = `<span>Connecting... (${retry}/${maxRetry})</span>`;


        } catch (e) {
            console.error("Polling error:", e);
        }

    }, 2000);
}

async function updateWiFiUI() {

    const res = await fetch("/api/wifi/status");
    const data = await res.json();

    const statusEl = document.getElementById("wifiStatus");
    const ssidEl = document.getElementById("currentSSID");
    const ipEl = document.getElementById("ipAddress");
    const rssiEl = document.getElementById("signalStrength");
    const btn = document.getElementById("btn-connect-wifi");
    const mac = document.getElementById("mac");
    const uptime = document.getElementById("uptime"); 

    if (data.connected) {
        statusEl.innerText = "Connected";
        statusEl.className = "status-badge status-connected";

        ssidEl.innerText = data.ssid;
        ipEl.innerText = data.ip;
        rssiEl.innerText = data.rssi + " dBm";

        // ✅ thêm
        mac.innerText = data.mac || "-";
        uptime.innerText = data.uptime || "-";

        if (btn) {
            btn.disabled = false;

            setTimeout(() => {
                btn.innerHTML = "<span>Connect</span>";
            }, 3000);
        }

    } else {
        statusEl.innerText = "Disconnected";
        statusEl.className = "status-badge status-disconnected";

        ssidEl.innerText = "-";
        ipEl.innerText = "-";
        rssiEl.innerText = "-";

        mac.innerText = "-";
        uptime.innerText = "-";
    }
}

async function rebootDevice() {
    if (!confirm("Are you sure you want to reboot device?")) return;

    await fetch("/api/reboot", { method: "POST" });

    alert("Device is rebooting...");
}

async function connectMQTT() {
    const btn = document.querySelector("#mqtt .btn-primary");

    if (btn) {
        btn.innerHTML = "<span>Connecting...</span>";
        btn.disabled = true;
    }

    try {
        await fetch("/api/mqtt/connect", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                server: document.getElementById("mqtt-server").value,
                port: parseInt(document.getElementById("mqtt-port").value),
                username: document.getElementById("mqtt-user").value,
                password: document.getElementById("mqtt-pass").value,
                topic: document.getElementById("mqtt-topic").value
            })
        });

        pollMQTTStatus();

    } catch (e) {
        console.error(e);

        if (btn) {
            btn.innerHTML = "<span>Connect</span>";
            btn.disabled = false;
        }
    }
}

let mqttInterval;

function pollMQTTStatus() {
    let retry = 0;
    const maxRetry = 5; // ~20s

    const btn = document.querySelector("#mqtt .btn-primary");

    // clear polling cũ
    if (mqttInterval) clearInterval(mqttInterval);

    mqttInterval = setInterval(async () => {
        retry++;

        try {
            const res = await fetch("/api/mqtt/status");
            const data = await res.json();

            updateMQTTUI(data);

            // CONNECTED
            if (data.connected) {
                clearInterval(mqttInterval);

                if (btn) {
                    btn.innerHTML = "<span> Connect</span>";
                    btn.disabled = false;
                }

                return;
            }

            // FAIL (timeout)
            if (retry >= maxRetry) {
                clearInterval(mqttInterval);

                console.warn("MQTT connect failed");

                if (btn) {
                    btn.innerHTML = "<span> Connect</span>";
                    btn.disabled = false;
                }

                return;
            }

            if (btn) {
                btn.innerHTML = `<span>Connecting... (${retry}/${maxRetry})</span>`;
            }

        } catch (e) {
            console.error(e);
        }

    }, 2000);
}

function updateMQTTUI(data) {
    const statusEl = document.querySelector("#mqtt .status-badge");
    const lastMsgEl = document.querySelector("#mqtt .info-value");

    if (data.connected) {
        statusEl.innerText = "Connected";
        statusEl.className = "status-badge status-connected";
    } else {
        statusEl.innerText = "Disconnected";
        statusEl.className = "status-badge status-disconnected";
    }

    lastMsgEl.innerText = data.lastMessage || "-";
}

async function toggleLED1(el) {
    const state = el.checked;

    try {
        if (state) {
            await fetch("/api/on1");
        } else {
            await fetch("/api/off1");
        }

        console.log("LED1:", state ? "ON" : "OFF");

    } catch (e) {
        console.error("LED control error:", e);
        el.checked = !state; // rollback UI nếu lỗi
    }
}