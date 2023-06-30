window.dashboardMap = {
    lastGpsTimestamp: null,
    map: null,
    roberta: null,
    dashBack: 'map',
    imageId: 'robertaTop',

    initializeMap: function (elementId, mapKey) {
        const defCenter = [-75.507144, 40.229057];

        this.map = new atlas.Map(elementId, {
            authOptions: {
                authType: 'subscriptionKey',
                subscriptionKey: mapKey
            },
            center: defCenter,
            style: 'satellite',
            zoom: 19,
        });

        this.roberta = new atlas.HtmlMarker({
            id: this.imageId,
            htmlContent: `<img id="${this.imageId}" src="../images/roberta-top.webp" alt="Roberta" />`,
            position: defCenter,
        });
        this.map.markers.add(this.roberta);
    },

    updateRobertaLocation: function (gpsState) {
        if (gpsState.timestamp < this.lastGpsTimestamp) return;

        this.roberta.setOptions({
            position: [gpsState.longitude, gpsState.latitude]
        });

        var markerElement = document.getElementById(this.imageId);
        markerElement.style.transform = `rotate(${gpsState.heading}deg)`;

        this.lastGpsTimestamp = gpsMixerState.timestamp;
    },

    toggleView: function () {
        var map = document.getElementById('dashMap');
        var cam = document.getElementById('dashCam');
        if (dashboardMap.dashBack == 'map') {
            cam.className = 'dash-back';
            map.className = 'dash-front';
            dashboardMap.dashBack = 'cam';
        }
        else {
            map.className = 'dash-back';
            cam.className = 'dash-front';
            dashboardMap.dashBack = 'map';
        }
    }
};
