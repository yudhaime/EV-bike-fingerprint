#include "webdisplay.h"

const char* getWebPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>
body{
  font-family: Arial;
  margin:0;
  background:#f5f5f5;
  text-align:center;
}

.header{
  color:white;
  padding:20px;
  transition:0.2s;
  text-align:left;
}

.header-top{
  display:flex;
  justify-content:space-between;
  align-items:flex-start;
}

.header-left{
  flex:1;
}

.header-right{
  margin-left:15px;
}

h1{
  margin:0;
  font-size:20px;
  word-break:break-word;
}

.ssid-name{
  font-size:12px;
  opacity:0.8;
  margin-top:5px;
}

.switch{
  position:relative;
  display:inline-block;
  width:55px;
  height:28px;
}

.switch input{
  opacity:0;
  width:0;
  height:0;
}

.slider{
  position:absolute;
  cursor:pointer;
  top:0;
  left:0;
  right:0;
  bottom:0;
  background-color:rgba(255,255,255,0.3);
  transition:0.3s;
  border-radius:28px;
}

.slider:before{
  position:absolute;
  content:"";
  height:20px;
  width:20px;
  left:4px;
  bottom:4px;
  background-color:white;
  transition:0.3s;
  border-radius:50%;
}

input:checked + .slider{
  background-color:rgba(255,255,255,0.8);
}

input:checked + .slider:before{
  transform:translateX(27px);
}

.status-text{
  font-size:14px;
  margin-top:10px;
  font-weight:normal;
  text-align:left;
}

h2{margin-top:20px;}

.container{padding:15px;}

.button{
  width:100%;
  padding:15px;
  margin:10px 0;
  font-size:16px;
  border:none;
  border-radius:10px;
  cursor:pointer;
}

.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
}

.green{background:#2ecc71;color:white;}
.red{background:#e74c3c;color:white;}
.blue{background:#3498db;color:white;}
.gray{background:#bdc3c7;color:black;}
.orange{background:#e67e22;color:white;}

.card{
  background:white;
  padding:10px;
  margin:10px 0;
  border-radius:10px;
}

input{
  width:100%;
  padding:10px;
  margin:10px 0;
  border-radius:8px;
  border:1px solid #ccc;
  font-size:14px;
}
</style>
</head>

<body>

<div id="app"></div>

<script>

let currentState = "OFF";
let currentSource = "-";
let currentSsid = "";

function route(page){
  location.hash = page;
}

window.onhashchange = function(){
  if(location.hash === "#enroll") loadEnroll();
  else if(location.hash === "#manage") loadManage();
  else if(location.hash === "#wifi") loadWifi();
  else loadHome();
};

async function updateStatus(){
  try{
    let res = await fetch("/status");
    let data = await res.json();
    currentState = data.state;
    currentSource = data.source;
  }catch(e){}
  
  try{
    let res = await fetch("/getWifi");
    let data = await res.json();
    currentSsid = data.ssid;
  }catch(e){}
}

function renderHeader(title="Panel Kontrol"){
  let color = (currentState === "ON") ? "#2ecc71" : "#e74c3c";
  let isChecked = (currentState === "ON") ? "checked" : "";
  
  let displaySource = currentSource;
  if(currentState === "ON" && currentSource === "by web"){
    displaySource = "via web";
  }

  return `
  <div class="header" style="background:${color}">
    <div class="header-top">
      <div class="header-left">
        <h1>${title}</h1>
        <div class="ssid-name">${currentSsid}</div>
      </div>
      <div class="header-right">
        <label class="switch">
          <input type="checkbox" id="relayToggle" ${isChecked} onchange="toggleRelayFromSwitch()">
          <span class="slider"></span>
        </label>
      </div>
    </div>
    <div class="status-text">
      ${currentState}${currentState==="ON" ? " ("+displaySource+")" : ""}
    </div>
  </div>
  `;
}

async function toggleRelayFromSwitch(){
  let toggle = document.getElementById("relayToggle");
  if(!toggle) return;
  
  await fetch("/relay");
  
  setTimeout(async ()=>{
    await updateStatus();
    let newColor = (currentState === "ON") ? "#2ecc71" : "#e74c3c";
    let headerDiv = document.querySelector(".header");
    if(headerDiv) headerDiv.style.background = newColor;
    
    let newToggle = document.getElementById("relayToggle");
    if(newToggle){
      newToggle.checked = (currentState === "ON");
    }
    
    let statusDiv = document.querySelector(".status-text");
    if(statusDiv){
      let displaySource = currentSource;
      if(currentState === "ON" && currentSource === "by web"){
        displaySource = "via web";
      }
      statusDiv.innerText = currentState + (currentState==="ON" ? " ("+displaySource+")" : "");
    }
  }, 200);
}

async function syncHeader(){
  await updateStatus();
  let color = (currentState === "ON") ? "#2ecc71" : "#e74c3c";
  let headerDiv = document.querySelector(".header");
  if(headerDiv){
    headerDiv.style.background = color;
    
    let ssidDiv = headerDiv.querySelector(".ssid-name");
    if(ssidDiv) ssidDiv.innerHTML = currentSsid;
    
    let toggle = document.getElementById("relayToggle");
    if(toggle){
      toggle.checked = (currentState === "ON");
    }
    
    let statusDiv = headerDiv.querySelector(".status-text");
    if(statusDiv){
      let displaySource = currentSource;
      if(currentState === "ON" && currentSource === "by web"){
        displaySource = "via web";
      }
      statusDiv.innerText = currentState + (currentState==="ON" ? " ("+displaySource+")" : "");
    }
  }
}

async function loadHome(){
  await updateStatus();
  
  document.getElementById("app").innerHTML = `
    ${renderHeader("Panel Kontrol")}
    <div class="container">
      <h2>Setting Fingerprint</h2>
      <div class="grid">
        <button class="button green" onclick="route('enroll')">Tambah</button>
        <button class="button gray" onclick="route('manage')">Kelola</button>
      </div>
      <h2>Pengaturan Sistem</h2>
      <button class="button orange" onclick="route('wifi')">Setting WiFi</button>
    </div>
  `;
}

async function loadEnroll(){
  await updateStatus();
  
  document.getElementById("app").innerHTML = `
    ${renderHeader("Tambah Sidik Jari")}
    <div class="container">
      <div class="card">
        <p>1. Tekan tombol "Mulai"</p>
        <p>2. Modul berbunyi beep 4x</p>
        <p>3. Letakan sidik jari baru</p>
        <p>4. Ikuti instruksi di bawah</p>
      </div>
      <h3 id="status">Menunggu...</h3>
      <button class="button green" onclick="startEnroll()">Mulai</button>
      <button class="button gray" onclick="route('home')">Kembali</button>
    </div>
  `;
}

async function startEnroll(){
  await fetch("/enroll");
  let interval = setInterval(async ()=>{
    let res = await fetch("/enrollStatus");
    let data = await res.json();
    document.getElementById("status").innerText = data.msg;
    if(data.result === "SUCCESS"){
      alert("Berhasil!");
      clearInterval(interval);
      route("home");
    }
    if(data.result === "FULL"){
      alert("DATA PENUH");
      clearInterval(interval);
      route("manage");
    }
    if(data.result === "FAIL"){
      alert("Gagal!");
      clearInterval(interval);
      route("home");
    }
  }, 800);
}

async function loadManage(){
  await updateStatus();
  
  document.getElementById("app").innerHTML = `
    ${renderHeader("Kelola Sidik Jari")}
    <div class="container">
      <p>Loading...</p>
      <button class="button gray" onclick="route('home')">Kembali</button>
    </div>
  `;
  let res = await fetch("/list");
  let data = await res.json();
  let html = `${renderHeader("Kelola Sidik Jari")}<div class="container">`;
  data.forEach(user=>{
    html += `<div class="card"><b>ID ${user.id}</b><br>${user.name}<br><br>
    <div class="grid">
      <button class="button blue" onclick="renameUser(${user.id})">Rename</button>
      <button class="button red" onclick="deleteUser(${user.id})">Hapus</button>
    </div></div>`;
  });
  html += `<button class="button red" onclick="deleteAll()">Hapus Semua ID</button>
    <button class="button gray" onclick="route('home')">Kembali</button></div>`;
  document.getElementById("app").innerHTML = html;
}

async function deleteUser(id){
  if(confirm("Hapus ID " + id + "?")){
    await fetch("/delete?id=" + id);
    loadManage();
  }
}

async function deleteAll(){
  if(confirm("Hapus semua fingerprint?")){
    await fetch("/deleteAll");
    loadManage();
  }
}

async function renameUser(id){
  let name = prompt("Nama:");
  if(name){
    await fetch("/setname?id="+id+"&name="+name);
    loadManage();
  }
}

async function loadWifi(){
  await updateStatus();
  let res = await fetch("/getWifi");
  let data = await res.json();
  document.getElementById("app").innerHTML = `
    ${renderHeader("Pengaturan WiFi")}
    <div class="container">
      <div class="card" style="background:#fff3cd; border:1px solid #ffc107;">
        <p style="color:#856404; margin:0; font-size:14px;">PERINGATAN PENTING!</p>
        <p style="color:#856404; margin:5px 0 0 0; font-size:12px;">
          1. Mengganti setting ini akan MERESTART MODUL dengan settingan terbaru.<br>
          2. Pastikan Anda MENYIMPAN NAMA WiFi dan PASSWORD yang benar.<br>
          3. Jika modul TIDAK BISA TERHUBUNG ke WiFi baru, Anda perlu UPLOAD ULANG FIRMWARE.<br>
          4. Untuk kembali ke mode Access Point, upload ulang firmware.
        </p>
      </div>
      <div class="card">
        <label><b>SSID (Nama WiFi)</b></label>
        <input type="text" id="ssid" placeholder="Nama WiFi" value="${data.ssid}">
        <label><b>Password</b></label>
        <input type="password" id="pass" placeholder="Password WiFi (kosongkan jika open network)">
        <p style="color:red; font-size:12px;" id="warning"></p>
        <button class="button green" onclick="saveWifi()">Simpan dan Restart</button>
        <button class="button gray" onclick="route('home')">Kembali</button>
      </div>
    </div>
  `;
}

async function saveWifi(){
  let ssid = document.getElementById("ssid").value;
  let pass = document.getElementById("pass").value;
  if(ssid === ""){
    document.getElementById("warning").innerText = "SSID tidak boleh kosong!";
    return;
  }
  let confirmMsg = "PERINGATAN PENTING\n\nSSID baru: " + ssid + "\n";
  if(pass === "") confirmMsg += "Password: (kosong / open network)\n\n";
  else confirmMsg += "Password: " + pass + "\n\n";
  confirmMsg += "Setelah disimpan:\n- Modul akan RESTART otomatis\n- Anda harus menyambung ke WiFi baru: '" + ssid + "'\n- Jika gagal terhubung, Anda perlu UPLOAD ULANG FIRMWARE\n\nLanjutkan menyimpan?";
  if(confirm(confirmMsg)){
    document.getElementById("warning").innerText = "Menyimpan dan merestart modul...";
    document.getElementById("warning").style.color = "orange";
    document.getElementById("warning").style.fontWeight = "bold";
    let btn = document.querySelector(".button.green");
    if(btn) btn.disabled = true;
    await fetch("/setWifi?ssid=" + encodeURIComponent(ssid) + "&pass=" + encodeURIComponent(pass));
    setTimeout(()=>{
      alert("Modul sedang restart.\n\nSilakan sambungkan perangkat Anda ke WiFi:\n" + ssid);
      location.reload();
    }, 2000);
  }
}

setInterval(syncHeader, 400);

if(!location.hash || location.hash === "#"){
  location.hash = "home";
} else {
  window.onhashchange();
}
</script>
</body>
</html>
)rawliteral";
}
