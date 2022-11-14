const ws = new WebSocket('ws://localhost:3030');
let colorscale;


const generateGradient = (steps) => {
    return `linear-gradient(90deg,${Array.from(Array(steps)).map((val,i) => {return `rgb(${colorscale(i * (100/steps)).rgb()}) ${i * (100/steps)}%`})})`
}

const getLastGlobalColorDOM = (globalColorScaleValue) => {
    if(globalColorScaleValue) {
        return `<div style="width: 100px; height: 100px; background-color: rgb(${colorscale(globalColorScaleValue).rgb()})"></div>
        <input type="range" min="1" max="100" value="${globalColorScaleValue}" class="slider" style="background: ${generateGradient(20)}" onchange="wsSendGlobalColorValue(this.value)">`
    }
    return ``;
}

const getBallSliderDOM = (id, rgbValues) => {

    return `<div class="slidecontainer" id=${id}>
            id: ${id}
            <input type="range" min="1" max="255" value="${rgbValues.r}" class="slider slider--red" onchange="wsSendColor(this.value, this.parentNode.id, 'r')">
            <input type="range" min="1" max="255" value="${rgbValues.g}" class="slider slider--green" onchange="wsSendColor(this.value, this.parentNode.id, 'g')">
            <input type="range" min="1" max="255" value="${rgbValues.b}" class="slider slider--blue" onchange="wsSendColor(this.value, this.parentNode.id, 'b')">
          </div>`
}


const wsSendGlobalColorValue = (value) => {
    ws.send(JSON.stringify({globalColor: value}))
}

const wsSendColor = (value, id, colorName) => {
    ws.send(JSON.stringify({id: id, value: value, color: colorName}))
}

document.addEventListener("DOMContentLoaded", function(){
    colorscale = chroma.scale('Spectral').domain([0,100]);

    ws.onopen = () => {
        console.log('Now connected');
    };

    ws.onmessage = (event) => {
        console.log('event', event)
        const data = JSON.parse(event.data);
        console.log('data', data)
        document.getElementById("lastGlobalColor").innerHTML = getLastGlobalColorDOM(data.globalColorScaleValue);
        console.log(document.getElementById("lastGlobalColor"))
        if(data.balls) {
            document.getElementById("balls").innerHTML = data.balls.map(([id,val]) => getBallSliderDOM(id, val));
        }

        //document.getElementById("lastGlobalColor").innerHTML = "cghghghfghxcvb";
    };

});
