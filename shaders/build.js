const fs = require('fs');
const path = require('path');

const platforms = {
    "darwin": "osx",
    "win32": "windows",
    "linux": "linux"
};

const config = require(__dirname + "/config");

const libraryIntermediatePath = config.intermediatePath + '/src';
const exampleIntermediatePath = config.intermediatePath + '/example-src'
if (!fs.existsSync(config.intermediatePath)) {
    fs.mkdirSync(config.intermediatePath);
}
if (!fs.existsSync(libraryIntermediatePath)) {
    fs.mkdirSync(libraryIntermediatePath);
}
if (!fs.existsSync(exampleIntermediatePath)) {
    fs.mkdirSync(exampleIntermediatePath);
}

const command = require(config.scriptsPath + 'command');

function addBuildCommands(name,shaderFilePath,type,intermediatePath,buildCommands = []) {
    let profiles = [
        '120'   // OpenGL
    ];

    function addProfiles(platform) {
        config.profiles[platform][type].forEach((p) => {
            if (profiles.indexOf(p) == -1) {
                profiles.push(p);
            }
        });
    }

    addProfiles('common');
    addProfiles(process.platform);

    profiles.forEach((profile) => {
        let arrayName = `${ name }_${ type }_${ config.arrayName[profile] }`;
        let outFile = `${ arrayName }.bin.h`;
        let outPath = `${ intermediatePath }/${ outFile }`;
        
        let command = `${ config.shaderc } ` +
            `-f ${ shaderFilePath } ` +
            `-o ${ outPath } ` +
            `-i ${ config.shaderIncludePath } ` +
            `--type ${ type[0] } ` +
            `--platform ${ platforms[process.platform] } ` + 
            `-p ${ profile} ` +
            `--bin2c ${ arrayName }`;
        
        buildCommands.push(command);
    });

    return buildCommands;
}

function addCommandsForShaderPath(shadersPath,shaderDir,intermediatePath,buildCommands) {
    let shaderPath = path.join(shadersPath,shaderDir);
    fs.readdirSync(shaderPath).forEach((shaderFileName) => {
        if (/\.fs\./.test(shaderFileName)) {
            addBuildCommands(shaderDir,path.join(shaderPath,shaderFileName),'fragment',intermediatePath,buildCommands);
        }
        else if (/\.vs\./.test(shaderFileName)) {
            addBuildCommands(shaderDir,path.join(shaderPath,shaderFileName),'vertex',intermediatePath,buildCommands);
        }
    });
}

function generateBuildCommands(shadersPath,intermediatePath) {
    let buildCommands = [];
    if (process.argv.length>2) {
        process.argv.splice(2).forEach((shaderName) => {
            if (fs.existsSync(path.join(shadersPath,shaderName))) {
                addCommandsForShaderPath(shadersPath,shaderDir,intermediatePath,buildCommands);
            }
            else {
                console.error("WARNING: No such shader directory " + shaderName);
            }
        });
    }
    else {
        fs.readdirSync(shadersPath).forEach((shaderDir) => {
            addCommandsForShaderPath(shadersPath,shaderDir,intermediatePath,buildCommands);
        });
    }
    return buildCommands;
}

function executeBuildCommand(buildCommand, promises) {
    promises.push(
        new Promise((resolve,reject) => {
            command.exec(buildCommand)
                .then((stdout) => {
                    resolve();
                })
                .catch((err) => {
                    console.error(err.message);
                    resolve();
                });
        })
    );
}

function buildOutputFile(intermediatePath,outTemplate,outFile) {
    let buffer = "";
    fs.readdirSync(intermediatePath).forEach((intermediateFile) => {
        let intermediateFilePath = path.join(intermediatePath,intermediateFile);
        buffer += fs.readFileSync(intermediateFilePath, { encoding: 'utf-8'}) + "\n\n";
    });

    let headerFile = fs.readFileSync(outTemplate,{ encoding: 'utf-8' });
    headerFile = headerFile.replace("%SHADER_ARRAYS%",buffer);
    fs.writeFileSync(outFile, headerFile);
}

// Generate command list
let libraryBuildCommands = generateBuildCommands(config.shadersPath,libraryIntermediatePath);
let examplesBuildCommands = generateBuildCommands(config.exampleShadersPath,exampleIntermediatePath);


// Execute commands
let libraryBuildPromises = [];
libraryBuildCommands.forEach((buildCommand) => executeBuildCommand(buildCommand,libraryBuildPromises));

let exampleBuildPromises = [];
examplesBuildCommands.forEach((buildCommand) => executeBuildCommand(buildCommand,exampleBuildPromises));

Promise.all(libraryBuildPromises)
    .then(() => {
        // Create the include file from the template and the intermediate files
        console.log("Intermediate files build complete. Generating shader C header...");

        buildOutputFile(config.intermediatePath + "/src",config.outFileTemplate,config.outFile);
        
        return Promise.all(exampleBuildPromises);
    })

    .then(() => {
        // Create examples shader include file
        console.log("Intermediate example files build complete. Generating example shader C header...");
        buildOutputFile(config.intermediatePath + "/example-src",config.exampleOutFileTemplate,config.exampleOutFile);
    });


