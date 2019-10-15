const config = require('./deps-config.json');
const path = require('path');

require('./scripts/install-deps.js').install()
    .then(() => {
        return require('./script/dependencies').updateDependencies()
    })

    .then(() => {
        // From git repositories
        const git = require('./scripts/git');

        let promises = [];
        config.git.forEach((gitResource) => {
            promises.push(
                git.clone(gitResource.url,path.join('deps',gitResource.dest))
            )
        });

        return Promise.all(promises);
    })

    .then(() => {
        // From HTTP resources
        const download = require('./scripts/download.js');

        let depsDir = path.join(__dirname,'deps');
        return download.downloadFiles(config.http,depsDir);
    })

    .catch((err) => {
        console.error(err.message);
    });