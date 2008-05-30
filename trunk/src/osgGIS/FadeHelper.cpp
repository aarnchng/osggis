#include <osgGIS/FadeHelper>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/BlendFunc>
using namespace osgGIS;


/**
 * Vertex shader for fading
 */
static const char* fader_vertshader =
    "varying vec3 position; \n"
    "void main()\n"
    "{\n"
    "    position = gl_ModelViewMatrix * gl_Vertex;\n"
    "    gl_Position = ftransform();\n"
    "    gl_FrontColor = gl_Color;\n"
    "}\n";

/**
 * Fragment shader for fading
 */
static const char *fader_fragshader =
    "varying vec3 position;\n"
    "uniform float fade_in_dist;\n"
    "uniform float fade_out_dist;\n"
    "void main(void)\n"
    "{\n"
    "    float fade = 1.0 - smoothstep( fade_in_dist, fade_out_dist, length(position) );\n"
    "    gl_FragColor = vec4( gl_Color.rgb, gl_Color.a * fade );\n"
    "}\n";

void
FadeHelper::enableFading( osg::StateSet* state_set )
{    
    osg::Program* fade_program = new osg::Program();
    fade_program->addShader( new osg::Shader( osg::Shader::VERTEX, fader_vertshader ) );
    fade_program->addShader( new osg::Shader( osg::Shader::FRAGMENT, fader_fragshader ) );
    state_set->setAttributeAndModes( fade_program, osg::StateAttribute::ON );

    osg::BlendFunc* blend_func = new osg::BlendFunc();
    blend_func->setFunction( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    state_set->setAttributeAndModes( blend_func, osg::StateAttribute::ON );
    state_set->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
}

void
FadeHelper::setOuterFadeDistance( float range, osg::StateSet* set )
{
    set->addUniform(
        new osg::Uniform( "fade_out_dist", range ),
        osg::StateAttribute::ON );
}

void
FadeHelper::setInnerFadeDistance( float range, osg::StateSet* set )
{
    set->addUniform( 
        new osg::Uniform( "fade_in_dist", range ),
        osg::StateAttribute::ON );
}


