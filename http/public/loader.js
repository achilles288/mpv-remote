var loaderLoadingScreen;
var loaderBrowserWindow;
var loaderURLWindow;
var loaderFileInput;
var loaderStatus;
var loaderTimer;
var loaderStartTime;
var loaderLoading = false;
var loaderError = {code: 0, message: "", time: 0}




function loaderOnLoaded(data) {
  loaderStatus.innerHTML = "";
  window.clearInterval(loaderTimer);
  loaderLoading = false;
  loaderLoadingScreen.style.display = "none";
  
  let name = document.getElementById("media-name");
  name.innerHTML = (data.name.trim() != "") ? data.name : "Untitled";
  name.style.color = "inherit";
  
  let controllerButtons
    = document.getElementById("media-controller-buttons").children;
  
  for(var i=0; i<controllerButtons.length; i++)
    controllerButtons[i].classList.remove("disabled");
  
  let tools = document.getElementById("media-controller-tools");
  let timeLabel = document.getElementById("time-label");
  tools.style.display = "block";
  let hr = Math.floor(data.duration / 3600);
  let min = Math.floor(data.duration / 60) % 60;
  let sec = Math.floor(data.duration) % 60;
  hr = String(hr);
  min = String(min).padStart(2, "0");
  sec = String(sec).padStart(2, "0");
  timeLabel.innerHTML = "0:00:00 / " + hr + ":" + min + ":" + sec;
}




function loaderOnIdle(event) {
  let t = new Date().getTime();
  if(t - loaderStartTime > 31000) {
    loaderStatus.innerHTML = "Sever not responding";
    window.clearInterval(loaderTimer);
    loaderLoading = false;
    loaderLoadingScreen.style.display = "none";
    return;
  }
  
  fetch(
    "status",
    {
      method: "GET",
      credentials: "same-origin",
      headers: {"Accepts":"application/json"}
    }
  )
    .then(response => {
      if(response.status == 200)
        return Promise.resolve(response.json());
      else
        return Promise.reject(new Error("Error syncing"));
    })
    .then(data => {
      if(data.loaded)
        loaderOnLoaded(data);
      
      if(data.error.code != 0) {
        if(loaderError.time != data.error.time) {
          loaderStatus.innerHTML = data.error.message;
          window.clearInterval(loaderTimer);
          loaderLoading = false;
          loaderLoadingScreen.style.display = "none";
        }
      }
      loaderError = data.error;
    })
    .catch((error) => console.error(error));
}




function loaderLoad(url) {
  if(loaderLoading)
    return;
  
  let formData = new FormData();
  formData.append("command", 'open "' + url + '" --pause');
  loaderStatus.innerHTML = "";
  
  fetch(
    "command",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200) {
        loaderStartTime = new Date().getTime();
        loaderLoading = true;
        loaderLoadingScreen.style.display = "block";
        loaderTimer = window.setInterval(loaderOnIdle, 100);
      }
      else if(response.status == 401) {
        loaderStatus.innerHTML = "Error 401 (Unauthorized)";
      }
      else {
        loaderStatus.innerHTML = "Error";
      }
    })
    .catch((error) => console.error(error));
}




function loaderOnBrowserSubmit(event) {
  event.preventDefault();
  let formData = new FormData(loaderBrowserWindow);
  let url = formData.get("url");
  if(!url)
    return;
  loaderBrowserWindow.parentElement.style.display = "none";
  loaderLoad(url);
}




function loaderOnUpload(event) {
  let formData = new FormData();
  formData.append("blob", event.target.files[0]);
  loaderLoadingScreen.style.display = "block";
  
  fetch(
    "upload",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin",
      headers: {"Accepts":"application/json"}
    }
  )
    .then(response => {
      loaderLoadingScreen.style.display = "none";
      if(response.status == 200)
        return Promise.resolve(response.json());
      else {
        loaderStatus.innerHTML = "Error uploading";
        return Promise.reject(new Error("Error uploading"));
      }
    })
    .then(data => {
      loaderLoad(data.url);
    })
    .catch((error) => console.error(error));
}




function loaderOnURL(event) {
  loaderURLWindow.parentElement.style.display = "block";
}

function loaderOnURLSubmit(event) {
  event.preventDefault();
  let url = document.getElementById("media-loader-url-input").value;
  loaderURLWindow.parentElement.style.display = "none";
  loaderLoad(url);
}




function loaderCloseWindow(event) {
  event.target.parentElement.parentElement.style.display = "none";
}




window.addEventListener(
  "load",
  function() {
    let buttons = document.getElementById("media-loader-buttons").children;
    buttons[2].onclick = loaderOnURL;
    
    loaderLoadingScreen = document.getElementById("loading-screen");
    loaderBrowserWindow = document.getElementById("media-loader-browser-win");
    loaderURLWindow = document.getElementById("media-loader-url-win");
    loaderStatus = document.getElementById("media-loader-status");
    
    loaderBrowserWindow.onsubmit = loaderOnBrowserSubmit;
    loaderURLWindow.onsubmit = loaderOnURLSubmit;
    
    loaderBrowserWindow.getElementsByClassName("close")[0].onclick
      = loaderCloseWindow;
    
    loaderURLWindow.getElementsByClassName("close")[0].onclick
      = loaderCloseWindow;
    
    loaderFileInput = buttons[1].getElementsByTagName("input")[0];
    
    buttons[1].onclick = function(event) {
      if(loaderLoading)
        return;
      let clickEvent = new MouseEvent("click");
      loaderFileInput.dispatchEvent(clickEvent);
    };
    loaderFileInput.oninput = loaderOnUpload;
  }
);
