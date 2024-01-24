import React, { useEffect, useState } from 'react'
import { useAppDispatch, useAppSelector } from '../context';
import { selectRoberta } from '../features/roberta/robertaSlice';
import { isBrowser } from '@tallman/strong-strap';

const AzMap = () => {
    const [imageId] = useState("robertaTop");
    const [defCenter] = useState([-75.507144, 40.229057]);
    const [elementId] = useState("mapDiv");
    const [map, setMap]=useState();
    const [robertaMarker, setRobertaMarker]=useState();

    const robertaState = useAppSelector(selectRoberta);

    function waitForAtlas() {
        if (typeof atlas === 'undefined') {
            setTimeout(waitForAtlas, 1000);
        }
        else {
            const amk = process.env.GATSBY_AZ_MAP_KEY
            var newMap = new atlas.Map(elementId, {
                authOptions: {
                    authType: 'subscriptionKey',
                    subscriptionKey: amk,
                },
                center: defCenter,
                style: 'satellite',
                zoom: 19,
            });
            setMap(newMap);

            var newRobertaMarker = new atlas.HtmlMarker({
                id: imageId,
                htmlContent: "<img id='" + imageId + "' src='../../roberta-top.webp' style='width: 48px;' alt='Roberta' />",
                position: defCenter,
            });
            setRobertaMarker(newRobertaMarker);
            newMap.markers.add(newRobertaMarker);
        }
    }

    useEffect(() => {
        if (!isBrowser) return;

        const cssUrls = [
            "https://atlas.microsoft.com/sdk/javascript/mapcontrol/2/atlas.min.css",
            // "https://api.tiles.mapbox.com/mapbox-gl-js/v0.53.0/mapbox-gl.css"
        ]
        cssUrls.forEach(cssUrl => {
            const cssElement = document.querySelector('link[href="' + cssUrl + '"]');
            if (!cssElement) {
                const css = document.createElement('link');
                css.rel = 'stylesheet';
                css.href = cssUrl;
                document.head.appendChild(css);
            }
        });

        const jsUrls = [
            "https://atlas.microsoft.com/sdk/javascript/mapcontrol/2/atlas.min.js",
        ]
        jsUrls.forEach(jsUrl => {
            const jsElement = document.querySelector('script[src="' + jsUrl + '"]');
            if (!jsElement) {
                const js = document.createElement('script');
                js.src = jsUrl;
                document.head.appendChild(js);
            }
        });

        waitForAtlas();
    }, []);

    useEffect(() => {
        if (!map || !robertaMarker) return;

        map.setCamera({
            center: [robertaState.gpsState.longitude, robertaState.gpsState.latitude]
        })

        robertaMarker.setOptions({
            position: [robertaState.gpsState.longitude, robertaState.gpsState.latitude],
            htmlContent: "<img id='" + imageId + "' src='../../roberta-top.webp' style='width: 48px; transform: rotate(" + robertaState.gpsState.heading + "deg)' alt='Roberta' />",
        });
    }, [robertaState])

    return (
        <div id={elementId} style={{ height: "100%" }} />
    )
}

export default AzMap
