#include "webdisplay.h"

String getWebPage() {
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
  transition: 0.2s;
}

h1{margin:0;}
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

.card{
  background:white;
  padding:10px;
  margin:10px 0;
  border-radius:10px;
}
</style>
</head>

<body>

<div id="app"></div>

<script>

// ================= ROUTER =================
function route(page){
  location.hash = page;
}

window.onhashchange = function(){
  if(location.hash === "#enroll") loadEnroll();
  else if(location.hash === "#manage") loadManage();
  else loadHome();
};

// ================= HEADER =================
function renderHeader(state, source, title="FOX-S"){
  let color = (state === "ON") ? "#2ecc71" : "#e74c3c";

  return `
  <div class="header" style="background:${color}">
    <h1>${title}</h1>
    <p>${state}${state==="ON" ? " ("+source+")":""}</p>
  </div>
  `;
}

// ================= HOME =================
async function loadHome(){

  let res = await fetch("/status");
  let data = await res.json();

  document.getElementById("app").innerHTML = `
    ${renderHeader(data.state, data.source)}

    <div class="container">

      <button class="button blue" onclick="toggleRelay()">
        ${data.state==="ON"?"Matikan Motor":"Nyalakan Motor"}
      </button>

      <h2>Setting Fingerprint</h2>

      <div class="grid">
        <button class="button green" onclick="route('enroll')">Tambah</button>
        <button class="button gray" onclick="route('manage')">Kelola</button>
      </div>

    </div>
  `;
}

// ================= RELAY =================
async function toggleRelay(){
  await fetch("/relay");
  setTimeout(loadHome, 200);
}

// ================= ENROLL PAGE =================
function loadEnroll(){

  document.getElementById("app").innerHTML = `
    ${renderHeader("ON","", "Tambah Sidik Jari")}

    <div class="container">

      <div class="card">
        <p>1. Tempel jari ke sensor</p>
        <p>2. Lepas saat beep</p>
        <p>3. Tempel ulang</p>
      </div>

      <h3 id="status">Menunggu...</h3>

      <button class="button green" onclick="startEnroll()">Mulai</button>
      <button class="button gray" onclick="route('home')">Kembali</button>

    </div>
  `;
}

// ================= START ENROLL =================
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

// ================= MANAGE =================
async function loadManage(){

  document.getElementById("app").innerHTML = `
    ${renderHeader("ON","", "Kelola Sidik Jari")}
    <div class="container">
      <p>Loading...</p>
      <button class="button gray" onclick="route('home')">Kembali</button>
    </div>
  `;

  let res = await fetch("/list");
  let data = await res.json();

  let html = `
    ${renderHeader("ON","", "Kelola Sidik Jari")}
    <div class="container">
  `;

  data.forEach(user=>{
    html += `
    <div class="card">
      <b>ID ${user.id}</b><br>
      ${user.name}<br><br>

      <div class="grid">
        <button class="button blue" onclick="renameUser(${user.id})">Rename</button>
        <button class="button red" onclick="deleteUser(${user.id})">Hapus</button>
      </div>
    </div>
    `;
  });

  html += `
    <button class="button red" onclick="deleteAll()">Hapus Semua ID</button>
    <button class="button gray" onclick="route('home')">Kembali</button>
    </div>
  `;

  document.getElementById("app").innerHTML = html;
}

// ================= DELETE =================
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

// ================= RENAME =================
async function renameUser(id){
  let name = prompt("Nama:");
  if(name){
    await fetch("/setname?id="+id+"&name="+name);
    loadManage();
  }
}

// ================= 🔥 REALTIME RELAY SYNC =================
async function syncStatus(){
  try{
    let res = await fetch("/status");
    let data = await res.json();

    let header = document.querySelector(".header");
    if(header){
      header.style.background =
        data.state === "ON" ? "#2ecc71" : "#e74c3c";

      header.querySelector("p").innerText =
        data.state + (data.state==="ON" ? " ("+data.source+")" : "");
    }

  }catch(e){}
}

// ================= AUTO SYNC =================
setInterval(syncStatus, 400);

// ================= START =================
if(!location.hash){
  location.hash = "home";
} else {
  window.onhashchange();
}
</script>

</body>
</html>
)rawliteral";
}
