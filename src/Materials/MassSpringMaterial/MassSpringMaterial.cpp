
#include "MassSpringMaterial.h"
#include "Node.h"
#include <glm/gtc/type_ptr.hpp>


MassSpringMaterial::MassSpringMaterial(std::string name,Texture2D *t) :
	MaterialGL(name), m_texture(t)
{

	vp = new GLProgram(MaterialPath + "MassSpringMaterial/MassSpringMaterial-VS.glsl", GL_VERTEX_SHADER);
	fp = new GLProgram(MaterialPath + "MassSpringMaterial/MassSpringMaterial-FS.glsl", GL_FRAGMENT_SHADER);

	m_ProgramPipeline->useProgramStage(vp, GL_VERTEX_SHADER_BIT);
	m_ProgramPipeline->useProgramStage(fp, GL_FRAGMENT_SHADER_BIT);


	l_ViewProj = glGetUniformLocation(vp->getId(), "ViewProj");
	l_Model = glGetUniformLocation(vp->getId(), "Model");
	l_PosLum = glGetUniformLocation(vp->getId(), "PosLum");
	l_PosCam = glGetUniformLocation(vp->getId(), "PosCam");



	l_Phong = glGetUniformLocation(fp->getId(), "Phong");
	l_Albedo = glGetUniformLocation(fp->getId(), "diffuseAlbedo");
	l_specColor = glGetUniformLocation(fp->getId(), "specularColor");

	l_tex = glGetUniformLocation(fp->getId(), "ColorTex");
	if (g_bindlessTexturesSupported) {
		glProgramUniformHandleui64ARB(fp->getId(), l_tex, t->getHandle());
	} else {
		// Set texture unit 0 for traditional binding (will bind texture in render)
		glProgramUniform1i(fp->getId(), l_tex, 0);
	}


	param.albedo = glm::vec3(0.2, 0.7, 0.8);
	param.coeff = glm::vec4(0.2,0.8,1.0,100.0);
	param.specularColor = glm::vec3(1.0);


	physik.mass = 0.05f;
	physik.deltaTime = 0.0f;
	physik.kd_dampening = 15.0f;
	physik.ks_Stiffness = 5500.0f;
	physik.wind = 0.0f;
	physik.windFriction = 0.5f;





	l_Time = glGetUniformLocation(vp->getId(), "Time");





	updatePhong();
}

MassSpringMaterial::~MassSpringMaterial()
{

}

void MassSpringMaterial::render(Node* o)
{
	// Bind texture for traditional mode
	if (!g_bindlessTexturesSupported && m_texture) {
		glBindTextureUnit(0, m_texture->getId());
	}

	m_ProgramPipeline->bind();

	o->drawGeometry(GL_TRIANGLES);
	m_ProgramPipeline->release();
}
void MassSpringMaterial::animate(Node* o, const float elapsedTime)
{

	glm::mat4 viewproj = Scene::getInstance()->camera()->getProjectionMatrix() * Scene::getInstance()->camera()->getViewMatrix();

	glProgramUniformMatrix4fv(vp->getId(), l_ViewProj, 1, GL_FALSE, glm::value_ptr(viewproj));
	glProgramUniformMatrix4fv(vp->getId(), l_Model, 1, GL_FALSE, glm::value_ptr(o->frame()->getModelMatrix()));

	glProgramUniform3fv(vp->getId(), l_PosLum, 1,  glm::value_ptr(Scene::getInstance()->getNode("Light")->frame()->convertPtTo(glm::vec3(0.0,0.0,0.0),o->frame())));

	glProgramUniform3fv(vp->getId(), l_PosCam, 1, glm::value_ptr(Scene::getInstance()->camera()->frame()->convertPtTo(glm::vec3(0.0, 0.0, 0.0), o->frame())));

	auto now_time = std::chrono::high_resolution_clock::now();
	auto timevar = now_time.time_since_epoch();
	float millis = 0.001f*std::chrono::duration_cast<std::chrono::milliseconds>(timevar).count();
	

	glProgramUniform1fv(vp->getId(), l_Time, 1,&millis);
	

	/*Afin de d�coreller la simulation de l'afficahge , nous faisons ici 10 pas de simulation CPU avant d'afficher le r�sultat*/
	for (int i = 0;i < 10;i++)
		computeMassSpringAnimation((CustomModelGL*) o->getModel());


	/*Direction du vecteur up dans le rep�re de l'objet. A utiliser pour d�finir la direction de la force de gravit�*/
	up_direction = Scene::getInstance()->getSceneNode()->frame()->convertDirTo(glm::vec3(0.0, 1.0, 0.0), o->frame());

	/*Direction du vecteur Wind dans le rep�re de l'objet. A utiliser pour d�finir la direction de la force du vent*/
	wind_direction = Scene::getInstance()->getSceneNode()->frame()->convertDirTo(glm::vec3(0.0, 0.0, -1.0), o->frame());
	

	/*Appels GPU pour mettre a jour le tableau des normales et des positions sur le GPU*/
	((CustomModelGL*)o->getModel())->recomputeNormals();
	((CustomModelGL*)o->getModel())->updatePositions();
}
void MassSpringMaterial::computeMassSpringAnimation(CustomModelGL* m)
{
	/**************A COMPLETER *********************************************************
	* 
	/* - Mise a 0 des Forces appliqu�es sur chaque �lm�ment a ce pas de temps
	*  - Ajout de la gravit�
	*  - Une m�thode classique des simulations num�riques est d'ajouter une force d'ammortissement pour simuler la r�sistance de l'air : On peut l'approximer par damp  * mass * V
	*  - La force due au vent peut �tre d�finie comme la masse d'une particule multipli�e par sa vitesse par rapport au vent : mass * (V - W) avec W = wind * wind_direction) 
	*  -  La force de friction du tissu avec l'air est plus complexe et d�pend de la normale des surfaces : Un exemple d'approximation : -wFr * (N * (W - V))* N) 
	*  - Ajouter les forces des ressorts 
	* 
	*********************/


	/*Ajouter les forces des ressorts*/
	computeAllSpringForces(m);

			

	/********************
	**** On ajoute ici des contraintes externes pour laisser le drapeau accroch� au poteau. Dans cet exemple, on annule les forces appliqu�es a ces points . Il serait �galement int�ressant de changer au d�marrage la longueur ou la rigidit� des ressorts voisins a ces points.
	* 
	* */
		m->F[m->indice(0, 0)] = glm::vec3(0.0f);
		m->F[m->indice(0, 1)] = glm::vec3(0.0f);
		m->F[m->indice(0, m->m_nbElements - 2)] = glm::vec3(0.0f);
		m->F[m->indice(0, m->m_nbElements-1)] = glm::vec3(0.0f);

	updateSimulation(m);
}


void MassSpringMaterial::computeAllSpringForces(CustomModelGL* m)
{

	for (Spring s : m->springs)
	{
		computeSpringForce(m,s);
	}

}

void MassSpringMaterial::updateSimulation(CustomModelGL* m)
{
	/*A compl�ter*/
	
}

void MassSpringMaterial::computeSpringForce(CustomModelGL* m, Spring s)
{
	/**A compl�ter**/
}


void MassSpringMaterial::updatePhong()
{
	glProgramUniform4fv(fp->getId(), l_Phong, 1, glm::value_ptr(glm::vec4(param.coeff)));
	glProgramUniform3fv(fp->getId(), l_Albedo, 1, glm::value_ptr(param.albedo));
	glProgramUniform3fv(fp->getId(), l_specColor, 1, glm::value_ptr(param.specularColor));
}





void MassSpringMaterial::displayInterface()
{

	if (ImGui::TreeNode("MassSpring"))
	{
			ImGui::BeginGroup();
			ImGui::SliderFloat("Particule Mass", &physik.mass, 0.05f, 10.0f, "Mass = %.3f");
			ImGui::SliderFloat("DeltaTime", &physik.deltaTime, 0.0f, 0.001f, "DeltaTime = %.4f");
			ImGui::SliderFloat("Stiffness : ks", &physik.ks_Stiffness, 0.0f, 20000.0f, "Stiffness = %.2f");
			ImGui::SliderFloat("Dampening", &physik.kd_dampening, 0.0f, 20.0f, "Dampening = %.2f");
			ImGui::SliderFloat("Wind power", &physik.wind, 0.0f, 50.0f, "Wind = %.2f");
			ImGui::SliderFloat("Wind Friction", &physik.windFriction, 0.0f, 2.0f, "Wind Friction = %.2f");

			ImGui::EndGroup();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TreePop();
	}
	if (ImGui::TreeNode("PhongParameters"))
	{
	bool upd = false;
		upd = ImGui::SliderFloat("ambiant", &param.coeff.x, 0.0f, 1.0f, "ambiant = %.2f") || upd;
		upd = ImGui::SliderFloat("diffuse", &param.coeff.y, 0.0f, 1.0f, "diffuse = %.2f") || upd;
		upd = ImGui::SliderFloat("specular", &param.coeff.z, 0.0f, 2.0f, "specular = %.2f") || upd;
		upd = ImGui::SliderFloat("exposant", &param.coeff.w, 0.1f, 200.0f, "exposant = %f") || upd;
		ImGui::PushItemWidth(200.0f);
		upd = ImGui::ColorPicker3("Albedo", glm::value_ptr(param.albedo)) || upd;;
		ImGui::SameLine();
		upd = ImGui::ColorPicker3("Specular Color", glm::value_ptr(param.specularColor)) || upd;;
		ImGui::PopItemWidth();
		if (upd)
			updatePhong();
		ImGui::TreePop();
	}


}




