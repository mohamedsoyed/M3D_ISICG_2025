#include "lab_work_ssao.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "utils/read_file.hpp"
#include "imgui.h"
#include <iostream>
#include <random>

namespace M3D_ISICG
{
	const std::string LabWorkSSAO::_shaderFolder = "src/lab_works/lab_work_ssao/shaders/";

	LabWorkSSAO::~LabWorkSSAO()
	{
		_model.cleanGL();
		glDeleteProgram( _programGeometry );
		glDeleteProgram( _programSSAO );
		glDeleteProgram( _programBlur );
		glDeleteProgram( _programPostprocess );
		glDeleteProgram( _programLighting );
	}

	bool LabWorkSSAO::init()
	{
		std::cout << "Initializing SSAO Lab Work..." << std::endl;
		glClearColor( _bgColor.x, _bgColor.y, _bgColor.z, _bgColor.w );
		glEnable( GL_DEPTH_TEST );

		if ( !_initProgram() )
			return false;

		_initCamera();
		_initFramebuffers();
		_generateSSAOData();

		_model.load( "model", "data/models/sponza/sponza.obj" );
		_model._transformation = glm::scale( _model._transformation, Vec3f( 0.03f ) );

		std::cout << "SSAO Lab Work Initialized Successfully!" << std::endl;
		return true;
	}

	bool LabWorkSSAO::_initProgram()
	{
		// Compile and link geometry, SSAO, blur, lighting, and postprocess shaders
		_programGeometry	= createProgram( _shaderFolder + "geometry.vert", _shaderFolder + "geometry.frag" );
		_programSSAO		= createProgram( _shaderFolder + "ssao.vert", _shaderFolder + "ssao.frag" );
		_programBlur		= createProgram( _shaderFolder + "blur.vert", _shaderFolder + "blur.frag" );
		_programPostprocess = createProgram( _shaderFolder + "postprocess.vert", _shaderFolder + "postprocess.frag" );
		_programLighting	= createProgram( _shaderFolder + "lighting.vert", _shaderFolder + "lighting.frag" );

		return _programGeometry != GL_INVALID_INDEX && _programSSAO != GL_INVALID_INDEX
			   && _programBlur != GL_INVALID_INDEX && _programPostprocess != GL_INVALID_INDEX
			   && _programLighting != GL_INVALID_INDEX;
	}

	void LabWorkSSAO::_initCamera()
	{
		_camera.setScreenSize( 1280, 720 );
		_camera.setPosition( Vec3f( 0.f, 2.f, 5.f ) );

	}

	void LabWorkSSAO::_initFramebuffers()
	{
		// Create G-buffer
		glGenFramebuffers( 1, &_gBuffer );
		glBindFramebuffer( GL_FRAMEBUFFER, _gBuffer );

		// Position texture
		glGenTextures( 1, &_gPosition );
		glBindTexture( GL_TEXTURE_2D, _gPosition );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, 1280, 720, 0, GL_RGB, GL_FLOAT, nullptr );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gPosition, 0 );

		// Normal texture
		glGenTextures( 1, &_gNormal );
		glBindTexture( GL_TEXTURE_2D, _gNormal );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, 1280, 720, 0, GL_RGB, GL_FLOAT, nullptr );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _gNormal, 0 );

		// Albedo texture
		glGenTextures( 1, &_gAlbedo );
		glBindTexture( GL_TEXTURE_2D, _gAlbedo );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, _gAlbedo, 0 );

		// Depth texture
		GLuint depthRenderBuffer;
		glGenRenderbuffers( 1, &depthRenderBuffer );
		glBindRenderbuffer( GL_RENDERBUFFER, depthRenderBuffer );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1280, 720 );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer );

		GLenum attachments[ 3 ] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers( 3, attachments );

		// Lighting buffer
		glGenFramebuffers( 1, &_lightingBuffer );
		glBindFramebuffer( GL_FRAMEBUFFER, _lightingBuffer );

		GLuint lightingTexture;
		glGenTextures( 1, &lightingTexture );
		glBindTexture( GL_TEXTURE_2D, lightingTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, 1280, 720, 0, GL_RGB, GL_FLOAT, nullptr );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightingTexture, 0 );

		if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		{
			std::cerr << "Lighting buffer not complete!" << std::endl;
		}

		if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
			std::cerr << "G-buffer not complete!" << std::endl;

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	void LabWorkSSAO::_generateSSAOData()
	{
		// Generate SSAO kernel
		std::uniform_real_distribution<float> randomFloats( 0.0, 1.0 );
		std::default_random_engine			  generator;

		for ( unsigned int i = 0; i < 64; ++i )
		{
			Vec3f sample( randomFloats( generator ) * 2.0f - 1.0f,
						  randomFloats( generator ) * 2.0f - 1.0f,
						  randomFloats( generator ) );
			sample = glm::normalize( sample );
			sample *= randomFloats( generator );
			float scale = float( i ) / 64.0f;
			scale		= glm::mix( 0.1f, 1.0f, scale * scale );
			sample *= scale;
			_ssaoKernel.push_back( sample );
		}

		// Generate noise texture
		std::vector<Vec3f> ssaoNoise;
		for ( unsigned int i = 0; i < 16; i++ )
		{
			Vec3f noise( randomFloats( generator ) * 2.0f - 1.0f, randomFloats( generator ) * 2.0f - 1.0f, 0.0f );
			ssaoNoise.push_back( noise );
		}

		glGenTextures( 1, &_noiseTexture );
		glBindTexture( GL_TEXTURE_2D, _noiseTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[ 0 ] );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}

	void LabWorkSSAO::render()
	{
		// 1. Geometry Pass
		glBindFramebuffer( GL_FRAMEBUFFER, _gBuffer );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glUseProgram( _programGeometry );
		// Set camera and model matrices
		_model.render( _programGeometry );

		// 2. SSAO Pass
		glBindFramebuffer( GL_FRAMEBUFFER, _ssaoBuffer );
		glClear( GL_COLOR_BUFFER_BIT );
		glUseProgram( _programSSAO );
		// Bind G-buffer textures and noise
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, _gPosition );
		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, _gNormal );
		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, _noiseTexture );
		// Render SSAO quad

		// 3. Blur Pass
		glBindFramebuffer( GL_FRAMEBUFFER, _ssaoBlurBuffer );
		glClear( GL_COLOR_BUFFER_BIT );
		glUseProgram( _programBlur );
		// Bind SSAO texture
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, _ssaoBuffer );
		// Render blurred quad

		// 4. Lighting Pass
		glBindFramebuffer( GL_FRAMEBUFFER, _lightingBuffer );
		glClear( GL_COLOR_BUFFER_BIT );
		glUseProgram( _programLighting );

		// Bind G-buffer textures
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, _gPosition );

		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, _gNormal );

		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, _gAlbedo );

		glActiveTexture( GL_TEXTURE3 );
		glBindTexture( GL_TEXTURE_2D, _ssaoBlurBuffer );

		// Render lighting quad
		_renderQuad();


		// 5. Postprocessing Pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(_programPostprocess);
		// Bind final lighting texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _lightingBuffer);
		// Render postprocessing quad
	}

	//void LabWorkSSAO::update( const float p_deltaTime ) { _camera.update( p_deltaTime ); }

	
	void LabWorkSSAO::handleEvents( const SDL_Event & p_event )
	{
		if ( p_event.type == SDL_KEYDOWN )
		{
			switch ( p_event.key.keysym.scancode )
			{
			case SDL_SCANCODE_W: // Front
				_camera.moveFront( _cameraSpeed );
				break;
			case SDL_SCANCODE_S: // Back
				_camera.moveFront( -_cameraSpeed );
				break;
			case SDL_SCANCODE_A: // Left
				_camera.moveRight( -_cameraSpeed );
				break;
			case SDL_SCANCODE_D: // Right
				_camera.moveRight( _cameraSpeed );
				break;
			case SDL_SCANCODE_R: // Up
				_camera.moveUp( _cameraSpeed );
				break;
			case SDL_SCANCODE_F: // Bottom
				_camera.moveUp( -_cameraSpeed );
				break;
			case SDL_SCANCODE_SPACE: // Print camera info
				_camera.print();
				break;
			default: break;
			}
		}

		// Rotate when left click + motion (if not on Imgui widget).
		/* if ( p_event.type == SDL_MOUSEMOTION && p_event.motion.state & SDL_BUTTON_LMASK
			 && !ImGui::GetIO().WantCaptureMouse )
		{
			_camera.rotate( p_event.motion.xrel * _cameraSensitivity, p_event.motion.yrel * _cameraSensitivity );
		}*/
	}
	
	 GLuint LabWorkSSAO::createProgram( const std::string vertexPath, const std::string fragmentPath )
	{
		// Lire le code des shaders
		std::string vertexCode	 = readFile( vertexPath );
		std::string fragmentCode = readFile( fragmentPath );

		const char * vertexSource	= vertexCode.c_str();
		const char * fragmentSource = fragmentCode.c_str();

		// Compiler le shader de vertex
		GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
		glShaderSource( vertexShader, 1, &vertexSource, nullptr );
		glCompileShader( vertexShader );

		GLint success;
		glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &success );
		if ( !success )
		{
			char infoLog[ 512 ];
			glGetShaderInfoLog( vertexShader, 512, nullptr, infoLog );
			std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
			return GL_INVALID_INDEX;
		}

		// Compiler le shader de fragment
		GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
		glShaderSource( fragmentShader, 1, &fragmentSource, nullptr );
		glCompileShader( fragmentShader );

		glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &success );
		if ( !success )
		{
			char infoLog[ 512 ];
			glGetShaderInfoLog( fragmentShader, 512, nullptr, infoLog );
			std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
			return GL_INVALID_INDEX;
		}

		// Lier les shaders dans un programme
		GLuint program = glCreateProgram();
		glAttachShader( program, vertexShader );
		glAttachShader( program, fragmentShader );
		glLinkProgram( program );

		glGetProgramiv( program, GL_LINK_STATUS, &success );
		if ( !success )
		{
			char infoLog[ 512 ];
			glGetProgramInfoLog( program, 512, nullptr, infoLog );
			std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			return GL_INVALID_INDEX;
		}

		// Nettoyage des shaders
		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );

		return program;
	}
	void LabWorkSSAO::_renderQuad()
	{
		static GLuint quadVAO = 0;
		static GLuint quadVBO;
		if ( quadVAO == 0 )
		{
			float quadVertices[] = { // Positions        // Texture Coords
									 -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

									 -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,	 -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,	 1.0f, 1.0f
			};
			glGenVertexArrays( 1, &quadVAO );
			glGenBuffers( 1, &quadVBO );
			glBindVertexArray( quadVAO );
			glBindBuffer( GL_ARRAY_BUFFER, quadVBO );
			glBufferData( GL_ARRAY_BUFFER, sizeof( quadVertices ), &quadVertices, GL_STATIC_DRAW );
			glEnableVertexAttribArray( 0 );
			glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void *)0 );
			glEnableVertexAttribArray( 1 );
			glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void *)( 2 * sizeof( float ) ) );
		}
		glBindVertexArray( quadVAO );
		glDrawArrays( GL_TRIANGLES, 0, 6 );
		glBindVertexArray( 0 );
	}
	void LabWorkSSAO::animate( const float p_deltaTime )
	{
		
	}

	void LabWorkSSAO::displayUI()
	{
		ImGui::Begin( "SSAO Settings" );
		// Background color
		if ( ImGui::ColorEdit3( "Background Color", glm::value_ptr( _bgColor ) ) )
		{
			glClearColor( _bgColor.x, _bgColor.y, _bgColor.z, _bgColor.w );
		}

		// Camera speed
		ImGui::SliderFloat( "Camera Speed", &_cameraSpeed, 0.1f, 5.0f );

		// SSAO settings (e.g., sample kernel size, radius)
		ImGui::SliderInt( "Kernel Size", &_kernelSize, 1, 64 );
		ImGui::SliderFloat( "Radius", &_radius, 0.1f, 5.0f );

		ImGui::End();
	}

	void LabWorkSSAO::resize( const int p_width, const int p_height )
	{
		_camera.setScreenSize( p_width, p_height );
		glViewport( 0, 0, p_width, p_height );
		// Resize framebuffers if needed
		// Example: Resize G-buffer textures
		glBindTexture( GL_TEXTURE_2D, _gPosition );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, p_width, p_height, 0, GL_RGB, GL_FLOAT, nullptr );

		glBindTexture( GL_TEXTURE_2D, _gNormal );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, p_width, p_height, 0, GL_RGB, GL_FLOAT, nullptr );

		glBindTexture( GL_TEXTURE_2D, _gAlbedo );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr );

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

} // namespace M3D_ISICG
