var controllerPauseButton;
var controllerPaused = true;
var controllerSlider;
var controllerDuration;
var controllerTime;
var controllerTimer1 = -1;
var controllerTimer2 = -1;
var controllerTimeLabel;
var controllerLoaded = false;




function controllerClientSetEnabled(en) {
  let name = document.getElementById("media-name");
  name.style.color = (en) ? "inherit" : "transparent";
  
  let tools = document.getElementById("media-controller-tools");
  tools.style.display = (en) ? "block" : "none";
  
  let controllerButtons
    = document.getElementById("media-controller-buttons").children;
  
  if(en) {
    for(let i=0; i<controllerButtons.length; i++)
      controllerButtons[i].classList.remove("disabled");
  }
  else {
    for(let i=0; i<controllerButtons.length; i++)
      controllerButtons[i].classList.add("disabled");
    
    controllerClientSetPaused(true);
    controllerClientSetTime(0);
  }
}




function controllerClientSetTime(time, moveSlider=true) {
  if(time > controllerDuration)
    controllerTime = controllerDuration;
  else
    controllerTime = time;
  
  if(moveSlider) {
    let val = (time/controllerDuration) * 1000;
    controllerSlider.update({value:Math.floor(val)}, false);
  }
  let timeLabel = document.getElementById("time-label");
  let hr = Math.floor(time / 3600);
  let min = Math.floor(time / 60) % 60;
  let sec = Math.floor(time) % 60;
  hr = String(hr);
  min = String(min).padStart(2, "0");
  sec = String(sec).padStart(2, "0");
  let str = timeLabel.innerHTML.substr(7, 10);
  timeLabel.innerHTML = hr + ":" + min + ":" + sec + str;
}




function controllerClientSetPaused(paused) {
  controllerPaused = paused;
  
  if(paused) {
    controllerPauseButton.children[0].className = "fa fa-play";
    controllerPauseButton.onclick = ((event) => controllerSetPaused(false));
    if(controllerTimer1 != -1) {
      window.clearInterval(controllerTimer1);
      controllerTimer1 = -1;
    }
  }
  else {
    controllerPauseButton.children[0].className = "fa fa-pause";
    controllerPauseButton.onclick = ((event) => controllerSetPaused(true));
    if(controllerTimer1 == -1) {
      var func = ((event) => controllerClientSetTime(controllerTime + 1));
      controllerTimer1 = window.setInterval(func, 1000);
    }
  }
}




function controllerSync() {
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
      controllerLoaded = data.loaded;
      controllerClientSetEnabled(data.loaded);
      if(data.loaded) {
        let mediaName = "Untitled"
        if(data.name.trim() != "")
          mediaName = data.name;
        document.getElementById("media-name").innerHTML = mediaName;
        
        controllerDuration = data.duration;
        let timeLabel = document.getElementById("time-label");
        let hr = Math.floor(data.duration / 3600);
        let min = Math.floor(data.duration / 60) % 60;
        let sec = Math.floor(data.duration) % 60;
        hr = String(hr);
        min = String(min).padStart(2, "0");
        sec = String(sec).padStart(2, "0");
        let str = timeLabel.innerHTML.substr(0, 10);
        timeLabel.innerHTML = str + hr + ":" + min + ":" + sec;
        
        controllerClientSetTime(data.time);
        controllerClientSetPaused(data.paused);
      }
    })
    .catch((error) => console.error(error));
}




function controllerSetPaused(paused) {
  if(controllerLoaded == false)
    controllerSync();
  
  let formData = new FormData();
  formData.append("command", "pause " + (paused ? "1" : "0"));
  
  fetch(
    "command",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200)
        controllerClientSetPaused(paused);
      else
        controllerClientSetEnabled(false);
    })
    .catch((error) => console.error(error));
}




function controllerMove(time) {
  if(controllerLoaded == false)
    controllerSync();
  
  let formData = new FormData();
  formData.append("command", "move " + time);
  
  fetch(
    "command",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200)
        controllerClientSetTime(controllerTime + time);
      else
        controllerClientSetEnabled(false);
    })
    .catch((error) => console.error(error));
}




function controllerSeek(time) {
  if(controllerLoaded == false)
    controllerSync();
  
  let formData = new FormData();
  formData.append("command", "seek " + time);
  
  fetch(
    "command",
    {
      method: "POST",
      body: formData,
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200)
        controllerClientSetTime(time, false);
      else
        controllerClientSetEnabled(false);
    })
    .catch((error) => console.error(error));
}




window.addEventListener(
  "load",
  function() {
    let buttons = document.getElementById("media-controller-buttons").children;
    buttons[0].onclick = ((event) => controllerMove(-15));
    buttons[2].onclick = ((event) => controllerMove(15));
    controllerPauseButton = buttons[1];
    
    controllerTimeLabel = document.getElementById("time-label");
    let slider = document.getElementById("time-control");
    controllerSlider = new rangesliderJs.RangeSlider(
      slider,
      {
        min: 0,
        max: 1000,
        value: 0,
        step: 1,
        
        onSlideStart: (val, percent, pos) => {
          if(controllerPaused == false && controllerTimer1 != -1) {
            window.clearInterval(controllerTimer1);
            controllerTimer1 = -1;
          }
        },
        
        onSlide: (val, percent, pos) => {
          controllerSeek(controllerDuration * percent);
        },
        
        onSlideEnd: (val, percent, pos) =>  {
          if(controllerPaused == true && controllerTimer1 == -1) {
            var func = ((event) => {
              controllerClientSetTime(controllerTime + 1);
            });
            controllerTimer1 = window.setInterval(func, 1000);
          }
        }
      }
    );
    
    controllerSync();
    var func = ((event) => controllerSync());
    controllerTimer2 = window.setInterval(func, 5000);
  }
);
