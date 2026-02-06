

#pragma once


#include "MaterialGL.h"

#include "CustomModelGL.h"
#include "Texture2D.h"



struct MSParam
{
	float ks_Stiffness;
	float kd_dampening;
	float mass;
	float deltaTime;
	float wind;
	float windFriction;
};


class MassSpringMaterial : public MaterialGL
{
	struct Phong {
		glm::vec4 coeff;
		glm::vec3 albedo;
		glm::vec3 specularColor;
	};
public:

	//Attributs

	//Constructeur-Destructeur

	/**
		Constructeur de la classe � partir du nom du mat�riau
		@param name : nom du mat�riau
	*/
	MassSpringMaterial(string name,Texture2D *t);

	/**
		Destructeur de la classe
	*/
	~MassSpringMaterial();

	//M�thodes

	/**
		M�thode virtuelle qui est appel�e pour faire le rendu d'un objet en utilisant ce mat�riau
		@param o : Node/Objet pour lequel on veut effectuer le rendu
	*/
	virtual void render(Node* o);

	/**
		M�thode virtuelle qui est appel�e pour modifier une valeur d'un param�tre n�cessaire pour le rendu
		@param o : Node/Objet concern� par le rendu
		@param elapsedTime : temps
	*/
	virtual void animate(Node* o, const float elapsedTime);



	 void updatePhong();

	virtual void displayInterface() ;



	/**
	* M�thode va g�rer la simulation au niveau global. Cette m�thode est responsable de calculer toutes les forces a appliquer sur chaque sommet et de mettre a jour la simulation en appelant updateSimulation.
	* Les forces a ajouter sont :
	*	La gravit�
	*	L'amortissement
	*	Le vent et sa friction
	*	Les ressorts entre chaque sommet
		@param m : le ModelGL custom a mettre a jour
	*/

	void computeMassSpringAnimation(CustomModelGL* m);


	/**
* Cette m�thode it�re sur tous les ressorts du mod�le et appele pour chaque la fonction computeSpringForce
	@param m : le ModelGL custom a mettre a jour
*/
	void computeAllSpringForces(CustomModelGL* m);



	//void computeSpringForce(CustomModelGL* m, int i, int j, int n_i, int n_j,float kFactor = 1.0f);

	/**
	* Cette m�thode doit calculer le comportement du ressort s. Elle doit notamment calculer pour les sommets id1 et id2 a chaque bout du ressort la force appliqu�e par le ressort. Cette fonction doit remplir le tableau F du mod�le pour les �l�ments id1 et id2.
	* La structure du ressort s est la suivante :
	* {
	*		int id1;	// Indice du sommet 1
	*		int id2;	// Indice du sommet 2
	*		float length; // longueur au repos
	*		float KsFactor; // modulation du facteur de rigidit� global. Ce facteur va "moduler" le facteur de rigidit� globale pass� en param�tre. Pour chaque ressort s, le facteur de rigidit� total est KsFactor * physik.ks_Stiffness
	*	}
	@param m : le ModelGL custom a mettre a jour
	@param s : le resort en question
	*/

	void computeSpringForce(CustomModelGL* m,Spring s);


	/**
	* M�thode appel�e pour mettre a jour la simulation de l'objet. Cette m�tohde repose sur l'int�gration semi-implicite d'Euler.
	* Cette m�thode doit calculer et mettre a jour la vitesse (tableau V) ainsi que la position (tableau listVertex de l'objet geometricModel) de chacun des �l�ments de chacun des �l�ments du mod�le m.
		@param m : le ModelGL custom a mettre a jour
	*/

	void updateSimulation(CustomModelGL* m);

protected:
	GLProgram* vp;
	GLProgram* fp;

	Texture2D* m_texture;  // Store texture for traditional binding fallback

	glm::dvec3 up_direction, wind_direction;

	GLuint l_ViewProj, l_Model, l_PosLum, l_PosCam, l_Phong, l_Albedo, l_specColor, l_Time, l_tex;
	Phong param;
	MSParam physik;


};
