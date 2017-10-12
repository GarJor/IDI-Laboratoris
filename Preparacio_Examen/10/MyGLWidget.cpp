#include "MyGLWidget.h"

#include <iostream>

MyGLWidget::MyGLWidget (QWidget* parent) : QOpenGLWidget(parent)
{
  setFocusPolicy(Qt::ClickFocus);  // per rebre events de teclat
  xClick = yClick = 0;
  angleY = 0.0;
  angleX = 0.0;
  anglerotate = 0.0f;
  maxfov = M_PI;
  perspectiva = true;
  pos2 = false;
  canvicolor = false;
  DoingInteractive = NONE;
}

MyGLWidget::~MyGLWidget ()
{
  if (program != NULL)
    delete program;
}

void MyGLWidget::initializeGL ()
{
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();  

  glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
  glEnable(GL_DEPTH_TEST);
  carregaShaders();
  createBuffers();
  calculradiEsc();

  inicamera();

  projectTransform ();
  viewTransform ();

  posFocus = glm::vec3(1.0, 1.0, 1.0);
  colFocus = glm::vec3(0.0, 1.0, 1.0); //COLOR CIAN
  //pintaVaca = 1;
  //glUniform1i(pintaVLoc, pintaVaca);
  glUniform3fv(posicioFLoc, 1, &posFocus[0]);
  glUniform3fv(colorFLoc, 1, &colFocus[0]);
}

void MyGLWidget::inicamera () {
    fov = float(M_PI)/3.0;
    fovi = fov;
    ra = 1.0f;
    //znear = radiEsc;
    //zfar = 3.0f*radiEsc;
    znear = 0.1f;
    zfar = 7.0f;

    left = -radiEsc;
    right = radiEsc;
    top = radiEsc;
    bottom = -radiEsc;
}

void MyGLWidget::paintGL () 
{
  // Esborrem el frame-buffer i el depth-buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Activem el VAO per a pintar el terra 
  glBindVertexArray (VAO_Terra);

  modelTransformTerra ();
  pintaVaca = 1;
  glUniform1i(pintaVLoc, pintaVaca);
  // pintem
  glDrawArrays(GL_TRIANGLES, 0, 12);

  // Activem el VAO per a pintar el Patricio
  glBindVertexArray (VAO_Patr);

  modelTransformPatricio ();
  pintaVaca = 1;
  glUniform1i(pintaVLoc, pintaVaca);
  // Pintem l'escena
  glDrawArrays(GL_TRIANGLES, 0, patr.faces().size()*3);
  
  glBindVertexArray(0);

  glBindVertexArray(VAO_Vaca);

  modelTransformVaca ();
  pintaVaca = 0;
  glUniform1i(pintaVLoc, pintaVaca);
  glDrawArrays(GL_TRIANGLES, 0, vaca.faces().size()*3);
}

void MyGLWidget::resizeGL (int w, int h) 
{
    glViewport(0, 0, w, h);

    float ravie  = float(w)/float(h);
    ra = ravie;

    if (ra < 1.0) {
      fov = 2.0 * glm::atan(glm::tan(fovi/2.0)/ra);
      left   = -radiEsc * ra;
      right  = radiEsc * ra;
      top    = radiEsc;
      bottom = -radiEsc;
    }

    if (ra > 1.0) {
       left   = -radiEsc;
       right  = radiEsc;
       top    = radiEsc/ra;
       bottom = -radiEsc/ra;
    }

    projectTransform();
}

void MyGLWidget::calculradiEsc () {

    escenaMax[0] = 2.0;
    escenaMax[1] = 2.0;
    escenaMax[2] = 2.0;

    escenaMin[0] = -2.0;
    escenaMin[1] = 0.0;
    escenaMin[2] = -2.0;

    centreEsc[0] = (escenaMax[0] + escenaMin[0])/2;
    centreEsc[1] = (escenaMax[1] + escenaMin[1])/2;
    centreEsc[2] = (escenaMax[2] + escenaMin[2])/2;

    float modx = pow(escenaMax[0] - centreEsc[0], 2);
    float mody = pow(escenaMax[1] - centreEsc[1], 2);
    float modz = pow(escenaMax[2] - centreEsc[2], 2);

    radiEsc = sqrt(modx + mody + modz);
}

void MyGLWidget::createBuffers ()
{
  // Carreguem el model de l'OBJ - Atenció! Abans de crear els buffers!
  patr.load("../models/Patricio.obj");
  vaca.load("../models/cow.obj");

  // Calculem la capsa contenidora del model
  calculaCapsaModel ();
  
  // Creació del Vertex Array Object del Patricio
  glGenVertexArrays(1, &VAO_Patr);
  glBindVertexArray(VAO_Patr);

  // Creació dels buffers del model patr
  // Buffer de posicions
  glGenBuffers(1, &VBO_PatrPos);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatrPos);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_vertices(), GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  // Buffer de normals
  glGenBuffers(1, &VBO_PatrNorm);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatrNorm);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_normals(), GL_STATIC_DRAW);

  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glGenBuffers(1, &VBO_PatrMatamb);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatrMatamb);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_matamb(), GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glGenBuffers(1, &VBO_PatrMatdiff);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatrMatdiff);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_matdiff(), GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glGenBuffers(1, &VBO_PatrMatspec);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatrMatspec);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3*3, patr.VBO_matspec(), GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glGenBuffers(1, &VBO_PatrMatshin);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatrMatshin);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*patr.faces().size()*3, patr.VBO_matshin(), GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  // Creació del Vertex Array Object de la Vaca
  glGenVertexArrays(1, &VAO_Vaca);
  glBindVertexArray(VAO_Vaca);

  // Creació dels buffers del model patr
  // Buffer de posicions
  glGenBuffers(1, &VBO_VacaPos);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_VacaPos);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vaca.faces().size()*3*3, vaca.VBO_vertices(), GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  // Buffer de normals
  glGenBuffers(1, &VBO_VacaNorm);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_VacaNorm);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vaca.faces().size()*3*3, vaca.VBO_normals(), GL_STATIC_DRAW);

  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glGenBuffers(1, &VBO_VacaMatamb);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_VacaMatamb);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vaca.faces().size()*3*3, vaca.VBO_matamb(), GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glGenBuffers(1, &VBO_VacaMatdiff);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_VacaMatdiff);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vaca.faces().size()*3*3, vaca.VBO_matdiff(), GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glGenBuffers(1, &VBO_VacaMatspec);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_VacaMatspec);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vaca.faces().size()*3*3, vaca.VBO_matspec(), GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glGenBuffers(1, &VBO_VacaMatshin);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_VacaMatshin);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vaca.faces().size()*3, vaca.VBO_matshin(), GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  // Dades del terra
  // VBO amb la posició dels vèrtexs
  glm::vec3 posterra[12] = {
	glm::vec3(-2.0, -1.0, 2.0),
	glm::vec3(2.0, -1.0, 2.0),
	glm::vec3(-2.0, -1.0, -2.0),
	glm::vec3(-2.0, -1.0, -2.0),
	glm::vec3(2.0, -1.0, 2.0),
	glm::vec3(2.0, -1.0, -2.0),
	glm::vec3(-2.0, -1.0, -2.0),
	glm::vec3(2.0, -1.0, -2.0),
	glm::vec3(-2.0, 1.0, -2.0),
	glm::vec3(-2.0, 1.0, -2.0),
	glm::vec3(2.0, -1.0, -2.0),
	glm::vec3(2.0, 1.0, -2.0)
  }; 

  // VBO amb la normal de cada vèrtex
  glm::vec3 norm1 (0,1,0);
  glm::vec3 norm2 (0,0,1);
  glm::vec3 normterra[12] = {
	norm1, norm1, norm1, norm1, norm1, norm1, // la normal (0,1,0) per als primers dos triangles
	norm2, norm2, norm2, norm2, norm2, norm2  // la normal (0,0,1) per als dos últims triangles
  };

  // Definim el material del terra
  glm::vec3 amb(0.2,0,0.2);
  glm::vec3 diff(0.8,0,0.8);
  glm::vec3 spec(0,0,0);
  float shin = 100;

  // Fem que aquest material afecti a tots els vèrtexs per igual
  glm::vec3 matambterra[12] = {
	amb, amb, amb, amb, amb, amb, amb, amb, amb, amb, amb, amb
  };
  glm::vec3 matdiffterra[12] = {
	diff, diff, diff, diff, diff, diff, diff, diff, diff, diff, diff, diff
  };
  glm::vec3 matspecterra[12] = {
	spec, spec, spec, spec, spec, spec, spec, spec, spec, spec, spec, spec
  };
  float matshinterra[12] = {
	shin, shin, shin, shin, shin, shin, shin, shin, shin, shin, shin, shin
  };

// Creació del Vertex Array Object del terra
  glGenVertexArrays(1, &VAO_Terra);
  glBindVertexArray(VAO_Terra);

  glGenBuffers(1, &VBO_TerraPos);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraPos);
  glBufferData(GL_ARRAY_BUFFER, sizeof(posterra), posterra, GL_STATIC_DRAW);

  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  glGenBuffers(1, &VBO_TerraNorm);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraNorm);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normterra), normterra, GL_STATIC_DRAW);

  // Activem l'atribut normalLoc
  glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(normalLoc);

  // En lloc del color, ara passem tots els paràmetres dels materials
  // Buffer de component ambient
  glGenBuffers(1, &VBO_TerraMatamb);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraMatamb);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matambterra), matambterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matambLoc);

  // Buffer de component difusa
  glGenBuffers(1, &VBO_TerraMatdiff);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraMatdiff);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matdiffterra), matdiffterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matdiffLoc);

  // Buffer de component especular
  glGenBuffers(1, &VBO_TerraMatspec);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraMatspec);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matspecterra), matspecterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matspecLoc);

  // Buffer de component shininness
  glGenBuffers(1, &VBO_TerraMatshin);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraMatshin);
  glBufferData(GL_ARRAY_BUFFER, sizeof(matshinterra), matshinterra, GL_STATIC_DRAW);

  glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(matshinLoc);

  glBindVertexArray(0);
}

void MyGLWidget::carregaShaders()
{
  // Creem els shaders per al fragment shader i el vertex shader
  QOpenGLShader fs (QOpenGLShader::Fragment, this);
  QOpenGLShader vs (QOpenGLShader::Vertex, this);
  // Carreguem el codi dels fitxers i els compilem
  fs.compileSourceFile("../10/shaders/fragshad.frag");
  vs.compileSourceFile("../10/shaders/vertshad.vert");
  // Creem el program
  program = new QOpenGLShaderProgram(this);
  // Li afegim els shaders corresponents
  program->addShader(&fs);
  program->addShader(&vs);
  // Linkem el program
  program->link();
  // Indiquem que aquest és el program que volem usar
  program->bind();

  // Obtenim identificador per a l'atribut “vertex” del vertex shader
  vertexLoc = glGetAttribLocation (program->programId(), "vertex");
  // Obtenim identificador per a l'atribut “normal” del vertex shader
  normalLoc = glGetAttribLocation (program->programId(), "normal");
  // Obtenim identificador per a l'atribut “matamb” del vertex shader
  matambLoc = glGetAttribLocation (program->programId(), "matamb");
  // Obtenim identificador per a l'atribut “matdiff” del vertex shader
  matdiffLoc = glGetAttribLocation (program->programId(), "matdiff");
  // Obtenim identificador per a l'atribut “matspec” del vertex shader
  matspecLoc = glGetAttribLocation (program->programId(), "matspec");
  // Obtenim identificador per a l'atribut “matshin” del vertex shader
  matshinLoc = glGetAttribLocation (program->programId(), "matshin");

  // Demanem identificadors per als uniforms del vertex shader
  transLoc = glGetUniformLocation (program->programId(), "TG");
  projLoc = glGetUniformLocation (program->programId(), "proj");
  viewLoc = glGetUniformLocation (program->programId(), "view");

  colorFLoc = glGetUniformLocation (program->programId(), "colFocus");
  posicioFLoc = glGetUniformLocation (program->programId(), "posFocus");
  pintaVLoc = glGetUniformLocation (program->programId(), "pintaVaca");
  pintarallesLoc = glGetUniformLocation (program->programId(), "pintaralles");
  tipusFocLoc = glGetUniformLocation (program->programId(), "tipusfocus");
}

void MyGLWidget::modelTransformPatricio ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació

  TG = glm::translate(TG, glm::vec3(1, -0.375, 0));
  TG = glm::rotate(TG, anglerotate, glm::vec3(0, 1, 0));
  TG = glm::scale(TG, glm::vec3(escala, escala, escala));
  TG = glm::translate(TG, -centrePatr);
  
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformVaca ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació

  TG = glm::translate(TG, glm::vec3(1,-0.75,0));
  TG = glm::rotate(TG, anglerotate, glm::vec3(0, 1, 0));
  TG = glm::rotate(TG, -float(M_PI)/2, glm::vec3(1,0,0));
  TG = glm::scale(TG, glm::vec3(escalavaca, escalavaca, escalavaca));
  TG = glm::translate(TG, -centreVaca);

  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::modelTransformTerra ()
{
  glm::mat4 TG(1.f);  // Matriu de transformació
  glUniformMatrix4fv (transLoc, 1, GL_FALSE, &TG[0][0]);
}

void MyGLWidget::projectTransform ()
{
  glm::mat4 Proj;  // Matriu de projecció
  if (perspectiva)
    Proj = glm::perspective(fov, ra, znear, zfar);
  else
    Proj = glm::ortho(left, right, bottom, top, znear, zfar);

  glUniformMatrix4fv (projLoc, 1, GL_FALSE, &Proj[0][0]);
}

void MyGLWidget::viewTransform ()
{
  //glm::mat4 View = glm::mat4(1.f);  // Matriu de posició i orientació
  /*View = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -2*radiEsc));
  View = glm::rotate(View, -angleY, glm::vec3(0, 1, 0));
  View = glm::rotate(View, -angleX, glm::vec3(1, 0, 0));
  glUniformMatrix4fv (viewLoc, 1, GL_FALSE, &View[0][0]);*/
  
  //lookAt(OBS, VRP, up)

  glm::vec3 VRP = glm::vec3(1, -0.375, 0);


  glm::mat4 View = glm::lookAt(glm::vec3(-1,1,-1), VRP, glm::vec3(0,1,0));
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &View[0][0]);
  
}

void MyGLWidget::calculaCapsaModel ()
{
  // Càlcul capsa contenidora i valors transformacions inicials
  float minx, miny, minz, maxx, maxy, maxz;
  minx = maxx = patr.vertices()[0];
  miny = maxy = patr.vertices()[1];
  minz = maxz = patr.vertices()[2];

  for (unsigned int i = 3; i < patr.vertices().size(); i+=3)
  {
    if (patr.vertices()[i+0] < minx)
      minx = patr.vertices()[i+0];
    if (patr.vertices()[i+0] > maxx)
      maxx = patr.vertices()[i+0];
    if (patr.vertices()[i+1] < miny)
      miny = patr.vertices()[i+1];
    if (patr.vertices()[i+1] > maxy)
      maxy = patr.vertices()[i+1];
    if (patr.vertices()[i+2] < minz)
      minz = patr.vertices()[i+2];
    if (patr.vertices()[i+2] > maxz)
      maxz = patr.vertices()[i+2];
  }
  escala = 0.25/(maxy-miny);
  centrePatr[0] = (minx+maxx)/2.0;
  centrePatr[1] = (miny+maxy)/2.0;
  centrePatr[2] = (minz+maxz)/2.0;

  
  /********************************VACA**************************************************/
  
  
  float  minxvaca, maxxvaca,  minyvaca, maxyvaca,  minzvaca, maxzvaca;
  minxvaca = maxxvaca = vaca.vertices() [0];
  minyvaca = maxyvaca = vaca.vertices() [1];
  minzvaca = maxzvaca = vaca.vertices() [2];
  
  for (unsigned int i = 3; i < vaca.vertices().size(); i+=3) {
	  
    if (vaca.vertices()[i+0] < minxvaca)
      minxvaca = vaca.vertices()[i+0];
    if (vaca.vertices()[i+0] > maxxvaca)
      maxxvaca = vaca.vertices()[i+0];
    if (vaca.vertices()[i+1] < minyvaca)
      minyvaca = vaca.vertices()[i+1];
    if (vaca.vertices()[i+1] > maxyvaca)
      maxyvaca = vaca.vertices()[i+1];
    if (vaca.vertices()[i+2] < minzvaca)
      minzvaca = vaca.vertices()[i+2];
    if (vaca.vertices()[i+2] > maxzvaca)
      maxzvaca = vaca.vertices()[i+2];
	
  }

  escalavaca = 0.5/(maxzvaca-minzvaca);
  centreVaca[0] = (minxvaca+maxxvaca)/2.0;
  centreVaca[1] = (minyvaca+maxyvaca)/2.0;
  centreVaca[2] = (minzvaca+maxzvaca)/2.0;

}

void MyGLWidget::keyPressEvent(QKeyEvent* event) 
{
  makeCurrent();
  switch (event->key()) {
    case Qt::Key_O: { // canvia òptica entre perspectiva i axonomètrica
      perspectiva = !perspectiva;
      projectTransform ();
      break;
    }
    
	case Qt::Key_R: {
        anglerotate += (float)M_PI/6.0;
		break;
	}
	      
  case Qt::Key_X: {
    canvicolor = !canvicolor;
    activa_ralles();
    break;
  }

  case Qt::Key_L: {
    pos2 = !pos2;
    canviaCamera();
    break;
  }

    default: event->ignore(); break;
  }
  update();
}

void MyGLWidget::canviaCamera () {

    if (pos2) {
        tipusfocus = 1;
        glUniform1i(tipusFocLoc, tipusfocus);
        posFocus = glm::vec3(1.0, 1.0, 1.0);
        glUniform3fv (posicioFLoc, 1, &posFocus[0]);
        colFocus = glm::vec3(0.8, 0.8, 0.8); //COLOR BLANC
        glUniform3fv(colorFLoc, 1, &colFocus[0]);
    }
    else {
        tipusfocus = 0;
        glUniform1i(tipusFocLoc, tipusfocus);
        posFocus = glm::vec3(1.0, 1.0, 1.0);
        glUniform3fv(posicioFLoc, 1, &posFocus[0]);
        colFocus = glm::vec3(0.0, 1.0, 1.0); //COLOR BLANC
        glUniform3fv(colorFLoc, 1, &colFocus[0]);
    }

}

void MyGLWidget::activa_ralles () {

    if (canvicolor) {
        pintaralles = 1;
        glUniform1i(pintarallesLoc, pintaralles);
    }

    else {
        pintaralles = 0;
        glUniform1i(pintarallesLoc, pintaralles);
    }

}

void MyGLWidget::mousePressEvent (QMouseEvent *e)
{
  xClick = e->x();
  yClick = e->y();

  if (e->button() & Qt::LeftButton &&
      ! (e->modifiers() & (Qt::ShiftModifier|Qt::AltModifier|Qt::ControlModifier)))
  {
    DoingInteractive = ROTATE;
  }
  if (e->button() & Qt::RightButton &&
      ! (e->modifiers() & (Qt::ShiftModifier|Qt::AltModifier|Qt::ControlModifier)))
  {
    DoingInteractive = ZOOM;
  }
  update();
}

void MyGLWidget::mouseReleaseEvent( QMouseEvent *)
{
  DoingInteractive = NONE;
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *e)
{
  makeCurrent();
  // Aqui cal que es calculi i s'apliqui la rotacio o el zoom com s'escaigui...
  if (DoingInteractive == ROTATE)
  {
    // Fem la rotació
    angleY += (e->x() - xClick) * M_PI / 180.0;
    angleX += (e->y() - yClick) * M_PI / 180.0;
    viewTransform ();
  }

  if (DoingInteractive == ZOOM)
  {
    // Fem zoom
      float aux = fov + (e->y() - yClick)/10.0;

      if (aux <= maxfov and aux > 0) {
        fov += (e->y() - yClick)/10.0;
     }

    emit CanviaSlider((fov/M_PI)*100);
    projectTransform ();
  }

  xClick = e->x();
  yClick = e->y();

  update ();
}

void MyGLWidget::canviFOV(int valor) {

    makeCurrent();

    float aux = ((float)valor/100)*M_PI;

    if (aux <= maxfov and aux > 0) {
        fov = ((float)valor/100)*M_PI;
    }

    createBuffers();
    projectTransform();
    
	update();
}



