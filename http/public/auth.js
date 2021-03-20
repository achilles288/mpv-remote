
function authOnFormSubmit(event) {
  event.preventDefault();
  
  fetch(
    "authenticate",
    {
      method: "POST",
      body: new FormData(event.target),
      credentials: "same-origin"
    }
  )
    .then(response => {
      if(response.status == 200) {
        event.target.getElementsByTagName("p")[0].innerHTML = "";
        event.target.parentElement.style.display = "none";
      }
      else if(response.status == 401) {
        event.target.getElementsByTagName("p")[0].innerHTML
          = "Authentication failed";
      }
      else {
        event.target.getElementsByTagName("p")[0].innerHTML = "Error";
      }
    })
    .catch((error) => console.error(error));
}




window.addEventListener(
  "load",
  function() {
    fetch(
      "is-authenticated",
      {
        method: "GET",
        credentials: "same-origin"
      }
    )
      .then(response => {
        if(response.status != 200) {
          let form = document.getElementById("auth-form");
          form.parentElement.style.display = "block";
        }
      })
      .catch((error) => console.error(error));
    
    let form = document.getElementById("auth-form");
    form.onsubmit = authOnFormSubmit;
  }
);
