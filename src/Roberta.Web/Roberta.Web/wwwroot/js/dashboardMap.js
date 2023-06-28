window.dashboardMap = {
    canvas: null,
    defCenter: null,
    lastGpsTimestamp: null,
    map: null,
    roberta: null,
    dashBack: 'map',

    initializeMap: function (elementId, apiKey) {
        window.dashboardMap.defCenter = new Microsoft.Maps.Location(40.229057, -75.507144);
        var center = window.dashboardMap.defCenter;

        var mapOptions = {
            allowHidingLabelsOfRoad: true,
            credentials: apiKey,
            center: window.dashboardMap.defCenter,
            disableMapTypeSelectorMouseOver: true,
            disableStreetside: true,
            disableStreetsideAutoCoverage: true,
            enableInertia: true,
            labelOverlay: 0,
            mapTypeId: Microsoft.Maps.MapTypeId.aerial,
            navigationBarMode: Microsoft.Maps.NavigationBarMode.minified,
            showDashboard: false,
            showScalebar: false,
            supportedMapTypes: [Microsoft.Maps.MapTypeId.aerial],
            zoom: 20
        };
        window.dashboardMap.map = new Microsoft.Maps.Map(document.getElementById(elementId), mapOptions);


        var polygonPoints = [
            new Microsoft.Maps.Location(center.latitude - 0.00002, center.longitude - 0.00002),
            new Microsoft.Maps.Location(center.latitude + 0.000005, center.longitude - 0.00002),
            new Microsoft.Maps.Location(center.latitude + 0.000005, center.longitude + 0.00002)
        ];
        var polygonOptions = {
            strokeColor: 'black',
            strokeThickness: 2,
            fillColor: 'red'
        };
        window.dashboardMap.roberta = new Microsoft.Maps.Polygon(polygonPoints, polygonOptions);
        window.dashboardMap.map.entities.push(window.dashboardMap.roberta);
    },

    updateRobertaLocation: function (gpsMixerState) {
        if (window.dashboardMap.lastGpsTimestamp > gpsMixerState.timestamp) return;

        //console.log(gpsMixerState);

        var leftLatitude = gpsMixerState.leftState.latitude;
        var leftLongitude = gpsMixerState.leftState.longitude;
        var rightLatitude = gpsMixerState.rightState.latitude;
        var rightLongitude = gpsMixerState.rightState.longitude;

        // Calculate midpoint coordinates
        var midLatitude = (leftLatitude + rightLatitude) / 2;
        var midLongitude = (leftLongitude + rightLongitude) / 2;

        // Calculate distance between left and right positions
        var distance = Math.sqrt(Math.pow(rightLatitude - leftLatitude, 2) + Math.pow(rightLongitude - leftLongitude, 2));

        // Calculate half the width and the height of the triangle
        var halfWidth = distance / 2;
        var height = 4 * halfWidth;

        // Calculate the bearing (heading) from midpoint towards top position
        var bearing = Math.atan2(rightLongitude - leftLongitude, rightLatitude - leftLatitude);
        if (bearing < 0) {
            bearing += 2 * Math.PI;
        }

        // Calculate the coordinates of the top position
        var topLatitude = midLatitude + (height * Math.cos(bearing));
        var topLongitude = midLongitude + (height * Math.sin(bearing));

        //console.log(distance + "|" + height + "|" + bearing);

        var newLocations = [
            new Microsoft.Maps.Location(gpsMixerState.leftState.latitude, gpsMixerState.leftState.longitude),
            new Microsoft.Maps.Location(topLatitude, topLongitude),
            new Microsoft.Maps.Location(gpsMixerState.rightState.latitude, gpsMixerState.rightState.longitude),
        ];
        window.dashboardMap.roberta.setLocations(newLocations);

        //    const newOptions = {
        //        center: {
        //            latitude: gpsMixerState.latitude,
        //            longitude: gpsMixerState.longitude,
        //        },
        //        zoom: 20,
        //    };
        //    window.mapInterop.map.setView(newOptions);

        window.dashboardMap.lastGpsTimestamp = gpsMixerState.timestamp;
    },

    addPoint: function (latitude, longitude) {
        var location = new Microsoft.Maps.Location(latitude, longitude);
        var pin = new Microsoft.Maps.Pushpin(location);
        window.dashboardMap.map.entities.push(pin);
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
