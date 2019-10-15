
const execCmd = require('./command').exec;

module.exports = {
    buildJS: function(path) {
        return new Promise((resolve,reject) => {
            if (process.platform=='win32') {
                console.log("Not implemented");
                reject("Not implemented");
            }
            else if (process.platform=='darwin') {
                execCmd(`bash ${ __dirname }/build.sh`).then(() => {
                    console.log("JavaScript build done");
                    resolve();
                });
            }
        });
    }
}