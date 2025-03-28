#include "GLCanvas.h"

GLCanvas::GLCanvas(QWidget *parent) : QOpenGLWidget(parent) {}

GLCanvas::~GLCanvas() = default;

void GLCanvas::initializeGL() {}
void GLCanvas::resizeGL(int w, int h) {}
void GLCanvas::paintGL() {}
