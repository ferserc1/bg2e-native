
const platforms = {
    "darwin": "osx",
    "win32": "win64",
    "linux": "linux"
};

module.exports = {
    scriptsPath: `${ __dirname }/../scripts/`,
    
    shaderc: `${ __dirname }/../deps/bgfx/${ platforms[process.platform] }/bin/shaderc`,
    
    shadersPath: `${ __dirname }/src`,
    outFileTemplate: `${ __dirname }/include/shaders.h.template`,
    outFile: `${ __dirname }/include/${ platforms[process.platform] }/shaders.h`,
    
    exampleShadersPath: `${ __dirname }/src-examples`,
    exampleOutFileTemplate: `${ __dirname }/include/example_shaders.h.template`,
    exampleOutFile: `${ __dirname }/include/${ platforms[process.platform] }/example_shaders.h`,

    intermediatePath: `${ __dirname }/include/intermediate`,
    shaderIncludePath: `${ __dirname }/lib`,
    
    arrayName: {
        "120": "glsl",
        vs_4_0: "dx11",
        ps_4_0: "dx11",
        vs_3_0: "dx9",
        ps_3_0: "dx9",
        spirv: "spv",
        metal: "mtl"
    },

    profiles: {
        common: {
            vertex: [
                '120'
            ],
            fragment: [
                '120'
            ]
        },
        win32: {
            vertex: [
                'vs_4_0',
                //'spirv',
                'vs_3_0'
            ],
            fragment: [
                'ps_4_0',
                //'spirv',
                'ps_3_0'
            ]
        },
        darwin: {
            vertex: [
                'metal',
                //'spirv'
            ],
            fragment: [
                'metal',
                //'spirv'
            ]
        },
        linux: {
            vertex: [
                //'spirv'
            ],
            fragment: [
                //'spirv'
            ]
        }
    }
}