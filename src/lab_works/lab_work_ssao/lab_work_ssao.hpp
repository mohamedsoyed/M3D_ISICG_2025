
#ifndef __LAB_WORK_SSAO_HPP__
#define __LAB_WORK_SSAO_HPP__

#include "GL/gl3w.h"
#include "common/base_lab_work.hpp"
#include "common/camera.hpp"
#include "common/models/triangle_mesh_model.hpp"
#include "define.hpp"
#include <map>
#include <vector>

namespace M3D_ISICG
{
	class LabWorkSSAO : public BaseLabWork
	{
	  public:
		LabWorkSSAO() : BaseLabWork() {}
		~LabWorkSSAO();

		bool init() override;
		void animate( const float p_deltaTime ) override;
		void render() override;
		void handleEvents( const SDL_Event & p_event ) override;
		void displayUI() override;
		void resize( const int p_width, const int p_height ) override;
		GLuint createProgram( const std::string vertexPath, const std::string fragmentPath );

	  private:
		bool _initProgram();
		void _initCamera();
		void _initFramebuffers();
		void _generateSSAOData();
		void _renderQuad();
		

		

	  private:
		TriangleMeshModel _model;
		Camera			  _camera;
		int				  _kernelSize = 64;	  // Nombre d'échantillons dans le noyau SSAO
		float			  _radius	  = 0.5f; // Rayon d'échantillonnage pour le SSAO

		// GL data
		GLuint _gBuffer		   = GL_INVALID_INDEX;
		GLuint _ssaoBuffer	   = GL_INVALID_INDEX;
		GLuint _ssaoBlurBuffer = GL_INVALID_INDEX;
		GLuint _lightingBuffer = GL_INVALID_INDEX;

		GLuint _gPosition = GL_INVALID_INDEX;
		GLuint _gNormal	  = GL_INVALID_INDEX;
		GLuint _gAlbedo	  = GL_INVALID_INDEX;

		GLuint _ssaoTexture		= GL_INVALID_INDEX;
		GLuint _ssaoBlurTexture = GL_INVALID_INDEX;

		GLuint _noiseTexture = GL_INVALID_INDEX;

		std::vector<Vec3f> _ssaoKernel;

		GLuint _programGeometry = GL_INVALID_INDEX;
		GLuint _programSSAO		= GL_INVALID_INDEX;
		GLuint _programBlur		= GL_INVALID_INDEX;
		GLuint _programLighting	   = GL_INVALID_INDEX;
		GLuint _programPostprocess = GL_INVALID_INDEX;

		// Shader uniform locations
		GLint _uMVPMatrixLoc		= GL_INVALID_INDEX;
		GLint _uViewMatrixLoc		= GL_INVALID_INDEX;
		GLint _uProjectionMatrixLoc = GL_INVALID_INDEX;

		Vec4f _bgColor		 = Vec4f( 0.1f, 0.1f, 0.1f, 1.f );
		Vec3f _lightPosition = Vec3f( 2.f, 4.f, -2.f );
		Vec3f _lightColor	 = Vec3f( 1.f, 1.f, 1.f );
		

		float _cameraSpeed		 = 0.1f;
		float _cameraSensitivity = 0.1f;

		static const std::string _shaderFolder;
	};
} // namespace M3D_ISICG

#endif // __LAB_WORK_SSAO_HPP__
