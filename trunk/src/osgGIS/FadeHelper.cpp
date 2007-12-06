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

//static const char* fader_w_lighting_vertshader =
//"varying vec3 position; "
//"void main() "
//"{ "
//"   vec3 normal, lightDir, viewVector, halfVector; "
//"	vec4 diffuse, ambient, globalAmbient, specular = vec4(0.0); "
//"	float NdotL, NdotHV; "
//	
//"	/* first transform the normal into eye space and normalize the result */ "
//"	normal = normalize(gl_NormalMatrix * gl_Normal); "
//	
//"	/* now normalize the light's direction. Note that according to the "
//"	OpenGL specification, the light is stored in eye space. Also since "
//"	we're talking about a directional light, the position field is actually "
//"	direction */ "
//"	lightDir = normalize(vec3(gl_LightSource[0].position)); "
//	
//"	/* compute the cos of the angle between the normal and lights direction.  "
//"	The light is directional so the direction is constant for every vertex. "
//"	Since these two are normalized the cosine is the dot product. We also  "
//"	need to clamp the result to the [0,1] range. */ "
//	
//"	NdotL = max(dot(normal, lightDir), 0.0); "
//	
//"	/* Compute the diffuse, ambient and globalAmbient terms */ "
//"	diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse; "
//"	ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient; "
//"	globalAmbient = gl_LightModel.ambient * gl_FrontMaterial.ambient; "
//	
//"	/* compute the specular term if NdotL is  larger than zero */ "
//"	if (NdotL > 0.0) { "
//"		NdotHV = max(dot(normal, normalize(gl_LightSource[0].halfVector.xyz)),0.0); "
//"		specular = gl_FrontMaterial.specular * gl_LightSource[0].specular * pow(NdotHV,gl_FrontMaterial.shininess); "
//"	} "
//	
//"	gl_FrontColor = globalAmbient + NdotL * diffuse + ambient + specular; "
//	
//"   position = gl_ModelViewMatrix * gl_Vertex; "
//"	gl_Position = ftransform(); "
//"}";


static const char* fader_w_lighting_vertshader =
"varying vec3 position; "
"void main() "
"{ "
"   vec3 normal, lightDir, viewVector, halfVector; "
"	vec4 diffuse, ambient, globalAmbient, specular = vec4(0.0); "
"	float NdotL, NdotHV; "
	
"	/* first transform the normal into eye space and normalize the result */ "
"	normal = normalize(gl_NormalMatrix * gl_Normal); "
	
"	/* now normalize the light's direction. Note that according to the "
"	OpenGL specification, the light is stored in eye space. Also since "
"	we're talking about a directional light, the position field is actually "
"	direction */ "
"	lightDir = normalize(vec3(gl_LightSource[0].position)); "
	
"	/* compute the cos of the angle between the normal and lights direction.  "
"	The light is directional so the direction is constant for every vertex. "
"	Since these two are normalized the cosine is the dot product. We also  "
"	need to clamp the result to the [0,1] range. */ "
	
"	NdotL = max(dot(normal, lightDir), 0.0); "
	
"	/* Compute the diffuse, ambient and globalAmbient terms */ "
"	diffuse = gl_Color * gl_LightSource[0].diffuse; "
"	ambient = gl_Color * gl_LightSource[0].ambient; "
"	globalAmbient = gl_LightModel.ambient * gl_Color; "
	
"	gl_FrontColor = globalAmbient + NdotL * diffuse + ambient + specular; "
	
"   position = gl_ModelViewMatrix * gl_Vertex; "
"	gl_Position = ftransform(); "
"}";


/**
 * Fragment shader for fading
 */
static const char *fader_fragshader = {
    "varying vec3 position;\n"
    "uniform float fade_in_dist;\n"
    "uniform float fade_out_dist;\n"
    "void main(void)\n"
    "{\n"
    "    float fade = 1.0 - smoothstep( fade_in_dist, fade_out_dist, length(position) );\n"
    "    gl_FragColor = vec4( gl_Color.rgb, gl_Color.a * fade );\n"
    "}\n"
};

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


