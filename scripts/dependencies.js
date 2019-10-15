const download = require("./download.js");
const path = require("path");

module.exports = {
    updateDependencies: function() {
        let downloadServer = 'vps396347.ovh.net';
        let dependencies = 'dependencies'

        let dependencyFiles = [
            { url:`http://${ downloadServer }/${ dependencies }/bullet-android-ios-macos-winVS2015-winVS2017.zip`, dest:"bullet" },
            { url:`http://${ downloadServer }/${ dependencies }/lua-macOS-iOS-winVS2015-winVS2017-src.zip`, dest:"lua" },
            { url:`http://${ downloadServer }/${ dependencies }/openal-android-winVS2015-winVS2017.zip`, dest:"openal" },
            //{ url:`http://${ downloadServer }/${ dependencies }/openvr-master.zip`, dest:"openvr-master" },
            { url:`http://${ downloadServer }/${ dependencies }/fbx2json-win64-osx.zip`, dest:"fbx2json-dist" },
            { url:`http://${ downloadServer }/${ dependencies }/bg2e-raytracer-win64-osx.zip`, dest:"bg2e-raytracer-dist" },
            { url:`http://${ downloadServer }/${ dependencies }/bg2e-scene-pkg-win64-osx.zip`, dest:"bg2e-scene-pkg" },
            { url:`http://${ downloadServer }/${ dependencies }/curl-win64-vs2017.zip`, dest:"curl" }
        ];

        let dependencyDir = path.join(__dirname,'../engine');

        return download.downloadFiles(dependencyFiles,dependencyDir);
    }
}