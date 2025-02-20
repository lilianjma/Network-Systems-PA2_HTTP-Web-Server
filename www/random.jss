document.addEventListener("DOMContentLoaded", function () {
    console.log("Page loaded successfully!");

    const button = document.querySelector("button");
    const outputDiv = document.getElementById("output");

    button.addEventListener("click", function () {
        outputDiv.textContent = "You clicked the button!";
        outputDiv.style.fontSize = "1.5em";
        outputDiv.style.color = "red";
    });

    setInterval(() => {
        console.log("This is a repeating message every 5 seconds.");
    }, 5000);
});
