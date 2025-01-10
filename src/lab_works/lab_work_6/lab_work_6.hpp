#ifndef __LAB_WORK_6_HPP__
#define __LAB_WORK_6_HPP__
#include "GL/gl3w.h"
#include "common/base_lab_work.hpp"
#include "common/camera.hpp"
#include "common/models/triangle_mesh.hpp"
#include "common/models/triangle_mesh_model.hpp"
#include "define.hpp"
#include <vector>

namespace M3D_ISICG
{
	class LabWork6 : public BaseLabWork
	{
	  public:
		LabWork6() : BaseLabWork() {}
		~LabWork6();

		bool init() override;
		void animate( const float p_deltaTime ) override;
		void render() override;

		void handleEvents( const SDL_Event & p_event ) override;
		void displayUI() override;

	  private:
		// ================ Scene data.

		Camera			  camera;
		TriangleMeshModel _bunny;

		void _initCamera();
		void _updateViewMatrix();

		float			  _cameraSpeed		 = 0.1f;
		float			  _cameraSensitivity = 0.1f;
		const std::string vertexShaderStr;	 
		const std::string fragmentShaderStr; 
		
		GLuint vertexShader, fragmentShader; 
		GLuint _geometryPassProgram;		
		GLuint _shadingPassProgram;
		GLint  location_MVP; 
		GLuint location_ViewMatrix, location_ModelMatrix, location_ProjectionMatrix;
		GLuint location_NormalMatrix;
		GLuint locationCameraPos;
		GLenum texture_a_afficher = GL_COLOR_ATTACHMENT1;
		GLuint _Vao;
		GLuint LightPOS;
		
		GLuint _gBufferTextures[ 6 ];
		GLuint FBO;

		void PreparerGBuffuer();

		void _geometryPass();
		void _shadingPass();

		void initialiser_geometryPassProgram();
		void initialiser_shadingPassProgram();

		void CreateCube();

		Mat4f			 MVPa;
		std::vector<int> Indices = { 0, 1, 2, 0, 1, 3 };

		std::vector<Vec2f> PositionsSommet
			= { Vec2f( 5.0f, 5.0f ), Vec2f( -5.0f, -5.0f ), Vec2f( 5.0f, -5.0f ), Vec2f( -5.0f, 5.0f ) };
		
		Vec4f _bgColor = Vec4f( 0.8f, 0.8f, 0.8f, 1.f ); 

		

		static const std::string _shaderFolder;
	};
} // namespace M3D_ISICG

#endif // __LAB_WORK_6_HPP__
