window.utils = {
    log: function (s) { console.log(s); },

    setVidSource: function() {
        const videoElement = document.getElementById('webcamVid');
        const videoUrl = 'http://rofo.mcgurkin.net:9080/stream'; // Replace with the actual URL

        // Set the source of the video element to the remote stream URL
        videoElement.src = videoUrl;
    },
};