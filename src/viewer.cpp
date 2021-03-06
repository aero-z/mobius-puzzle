// Name: Christian Zommerfelds
// Student Number: 20493973
// User-id: cgzommer

#include <GL/glew.h>
#include "viewer.hpp"
#include "algebra.hpp"
#include "level.hpp"
#include "game.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <boost/foreach.hpp>
#include <GL/glew.h>
#include <fstream>
#include <vector>
#include <sstream>
using namespace std;

namespace {

const int vboTexWidth = 768;

#define printOpenGLError() printOglError(__FILE__, __LINE__)

int printOglError(const char *file, int line)
{
    //
    // Returns 1 if an OpenGL error occurred, 0 otherwise.
    //
    GLenum glErr;
    int    retCode = 0;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
        retCode = 1;
        glErr = glGetError();
    }
    return retCode;
}

// draws a cube (front faces are counterclockwise)
void drawCube(double length) {
  glBegin(GL_QUADS);

  length *= 0.5;

  // comments are as if X-axis goes right, Y-axis goes away from viewer, Z-axis goes up

  // front face
  glNormal3d(0.0, -1.0, 0.0);
  glVertex3d(+length, -length, -length);
  glVertex3d(+length, -length, +length);
  glVertex3d(-length, -length, +length);
  glVertex3d(-length, -length, -length);
  // back face
  glNormal3d(0.0, +1.0, 0.0);
  glVertex3d(-length, +length, -length);
  glVertex3d(-length, +length, +length);
  glVertex3d(+length, +length, +length);
  glVertex3d(+length, +length, -length);
  // right face
  glNormal3d(+1.0, 0.0, 0.0);
  glVertex3d(+length, +length, -length);
  glVertex3d(+length, +length, +length);
  glVertex3d(+length, -length, +length);
  glVertex3d(+length, -length, -length);
  // left face
  glNormal3d(-1.0, 0.0, 0.0);
  glVertex3d(-length, -length, -length);
  glVertex3d(-length, -length, +length);
  glVertex3d(-length, +length, +length);
  glVertex3d(-length, +length, -length);
  // top face
  glNormal3d(0.0, 0.0, +1.0);
  glVertex3d(-length, -length, +length);
  glVertex3d(+length, -length, +length);
  glVertex3d(+length, +length, +length);
  glVertex3d(-length, +length, +length);
  // bottom face
  glNormal3d(0.0, 0.0, -1.0);
  glVertex3d(-length, +length, -length);
  glVertex3d(+length, +length, -length);
  glVertex3d(+length, -length, -length);
  glVertex3d(-length, -length, -length);

  glEnd();
}

// draws a cube (front faces are counterclockwise)
void drawParallelepiped(const Vector3D& p0, const Vector3D& v1, const Vector3D& v2, const Vector3D& v3) {
  glBegin(GL_QUADS);

  Vector3D p1 = p0 + v1;
  Vector3D p2 = p1 + v2;
  Vector3D p3 = p0 + v2;
  Vector3D p4 = p0 + v3;
  Vector3D p5 = p1 + v3;
  Vector3D p6 = p2 + v3;
  Vector3D p7 = p3 + v3;

  Vector3D n1 = v2.cross(v1);
  Vector3D n2 = v3.cross(v2);
  Vector3D n3 = v1.cross(v3);

  glNormal3dv(&n1[0]);
  glTexCoord2d(0,0);
  glVertex3d(p0[0], p0[1], p0[2]);
  glTexCoord2d(0,1);
  glVertex3dv(&p3[0]);
  glTexCoord2d(1,1);
  glVertex3dv(&p2[0]);
  glTexCoord2d(1,0);
  glVertex3dv(&p1[0]);

  glNormal3dv(&n2[0]);
  glTexCoord2d(0,0);
  glVertex3d(p0[0], p0[1], p0[2]);
  glTexCoord2d(0,1);
  glVertex3dv(&p4[0]);
  glTexCoord2d(1,1);
  glVertex3dv(&p7[0]);
  glTexCoord2d(1,0);
  glVertex3dv(&p3[0]);

  glNormal3dv(&n3[0]);
  glTexCoord2d(0,0);
  glVertex3d(p0[0], p0[1], p0[2]);
  glTexCoord2d(0,1);
  glVertex3dv(&p1[0]);
  glTexCoord2d(1,1);
  glVertex3dv(&p5[0]);
  glTexCoord2d(1,0);
  glVertex3dv(&p4[0]);

  glNormal3d(-n1[0], -n1[1], -n1[2]);
  glTexCoord2d(0,0);
  glVertex3dv(&p4[0]);
  glTexCoord2d(0,1);
  glVertex3dv(&p5[0]);
  glTexCoord2d(1,1);
  glVertex3dv(&p6[0]);
  glTexCoord2d(1,0);
  glVertex3dv(&p7[0]);

  glNormal3d(-n2[0], -n2[1], -n2[2]);
  glTexCoord2d(0,0);
  glVertex3dv(&p1[0]);
  glTexCoord2d(0,1);
  glVertex3dv(&p2[0]);
  glTexCoord2d(1,1);
  glVertex3dv(&p6[0]);
  glTexCoord2d(1,0);
  glVertex3dv(&p5[0]);

  glNormal3d(-n3[0], -n3[1], -n3[2]);
  glTexCoord2d(0,0);
  glVertex3dv(&p3[0]);
  glTexCoord2d(0,1);
  glVertex3dv(&p7[0]);
  glTexCoord2d(1,1);
  glVertex3dv(&p6[0]);
  glTexCoord2d(1,0);
  glVertex3dv(&p2[0]);

  glEnd();
}

}

Viewer::Viewer(Game& game)
: isGlInit (false),
  game (&game),
  camera (1, 0, 0),
  particleSys ( new ParticleSystem(game.getLevel()) ),
  enableSkyBox (true),
  enableParticleSystem (true),
  glow (2) {
    Glib::RefPtr<Gdk::GL::Config> glconfig;

    // Ask for an OpenGL Setup with
    //  - red, green and blue component colour
    //  - a depth buffer to avoid things overlapping wrongly
    //  - double-buffered rendering to avoid tearing/flickering
    glconfig = Gdk::GL::Config::create(
            Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH | Gdk::GL::MODE_DOUBLE);
    if (!glconfig) {
        // If we can't get this configuration, die
        std::cerr << "Unable to setup OpenGL Configuration!" << std::endl;
        abort();
    }

    // Accept the configuration
    set_gl_capability(glconfig);

    camera = Vector3D(1, 0, -1);
    camera.normalize();

    // Register the fact that we want to receive these events
    add_events(
            Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON2_MOTION_MASK
                    | Gdk::BUTTON3_MOTION_MASK | Gdk::BUTTON_PRESS_MASK
                    | Gdk::BUTTON_RELEASE_MASK | Gdk::VISIBILITY_NOTIFY_MASK);
}

Viewer::~Viewer() {
    // Nothing to do here right now.
}

void Viewer::invalidate() {
    //Force a rerender
    Gtk::Allocation allocation = get_allocation();
    get_window()->invalidate_rect(allocation, false);
}

Colour colours[4] = {
#if 1 // different colours per side
    Colour(1, 0, 1),
    Colour(0, 1, 1),
    Colour(1, 1, 0),
    Colour(0, 1, 0)
#else // all the same
    Colour(1, 1, 1),
    Colour(1, 1, 1),
    Colour(1, 1, 1),
    Colour(1, 1, 1)
#endif
};

void Viewer::setGame(Game& g) {
    game = &g;
    delete particleSys;
    particleSys = new ParticleSystem(game->getLevel() );
}

void Viewer::drawCurveBlock(const Segment& s, bool lightAndTex) {

    glColor3f(0.8, 0.8, 1);
    glDisable(GL_TEXTURE_2D);

    Vector3D q[4];
    Vector3D l[4];

    for (size_t i = 0; i < s.num(); i++) {
        const Vector3D& p = s.p(i);
        Vector3D n = s.n(i);
        const Vector3D& d = s.d(i);

        Vector3D e = d.cross(n);
        e.normalize();

#if 0   // draw normals
        glDisable(GL_LIGHTING);
        glBegin(GL_LINES);/*
        glColor3f(0, 1, 0);
        glVertex3dv(&p[0]);
        glVertex3d(p[0] + d[0], p[1] + d[1], p[2] + d[2]);*/
        glColor3f(0, 0, 1);
        glVertex3d(p[0], p[1], p[2]);
        glVertex3d(p[0] + n[0]/2, p[1] + n[1]/2, p[2] + n[2]/2);/*
        glColor3f(1, 0, 1);
        glVertex3dv(&p[0]);
        glVertex3d(p[0] + e[0], p[1] + e[1], p[2] + e[2]);*/
        glEnd();
        glColor3f(0.8, 0.8, 1);
#endif

        if (lightAndTex) {
            glEnable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex[0]);
        }
        else
            glDisable(GL_LIGHTING);

        glColor3f(1, 1, 1);

        n = radius * n;
        e = radius * e;

        l[0] = p + n + e;
        l[1] = p - n + e;
        l[2] = p - n - e;
        l[3] = p + n - e;

        if (i > 0) {

            double tex0 = (i-1) / (double) (s.num() - 1);
            double tex1 = i / (double) (s.num() - 1);

            glBegin(GL_QUADS);

            for (size_t c = 0; c < 4; c++) {

                if (lightAndTex) {

                    Colour& c1 = colours[c];
                    Colour& c2 = colours[(c-s.getSideDiff(1))%4];
                    Colour colour = c1 * (1-tex0) + c2 * (tex0); // blend the colours
                    glColor3f(colour.R(), colour.G(), colour.B());

                    /*if (c == 1 && s.isSwitched()) side = 3;
                    if (c == 3 && s.isSwitched()) side = 1;*/
                    //glColor3f(colours[side].R(), colours[side].G(), colours[side].B());
                }

                switch (c) {
                case 1: glNormal3dv(&e[0]); break;
                case 2: glNormal3d(-n[0],-n[1],-n[2]); break;
                case 3: glNormal3d(-e[0],-e[1],-e[2]); break;
                case 0: glNormal3d(n[0],n[1],n[2]); break;
                }

                glTexCoord2f (tex1, 0.0);
                glVertex3dv(&l[(c+3)%4][0]);
                glTexCoord2f (tex0, 0.0);
                glVertex3dv(&q[(c+3)%4][0]);
                glTexCoord2f (tex0, 0.2);
                glVertex3dv(&q[c][0]);
                glTexCoord2f (tex1, 0.2);
                glVertex3dv(&l[c][0]);

            }

            glEnd();
        }
        else { // i = 0
            if (s.adj[0] == NULL) {
                // draw front cap
                glColor3f(1, 0, 0);

                glBegin(GL_QUADS);

                glNormal3d(-d[0],-d[1],-d[2]);
                glTexCoord2f (0, 0);
                glVertex3dv(&l[0][0]);
                glTexCoord2f (0, 0.2);
                glVertex3dv(&l[3][0]);
                glTexCoord2f (0.2, 0.2);
                glVertex3dv(&l[2][0]);
                glTexCoord2f (0.2, 0);
                glVertex3dv(&l[1][0]);

                glEnd();
            }
        }

        for (size_t x = 0; x < 4; x++)
            q[x] = l[x];

        if (i == (s.num() - 1) && s.adj[1] == NULL) {
            // draw end cap
            glColor3f(1, 0, 0);

            glBegin(GL_QUADS);

            glNormal3d(d[0],d[1],d[2]);
            glTexCoord2f (0, 0);
            glVertex3dv(&l[1][0]);
            glTexCoord2f (0, 0.2);
            glVertex3dv(&l[2][0]);
            glTexCoord2f (0.2, 0.2);
            glVertex3dv(&l[3][0]);
            glTexCoord2f (0.2, 0);
            glVertex3dv(&l[0][0]);

            glEnd();
        }
    }
}

void drawT(Vector3D p, Vector3D d, const Vector3D& n, bool lightAndTex, size_t side) {

    Vector3D q[2];
    Vector3D s[2];

    Vector3D r[2];
    Vector3D l[2];

    Vector3D e = d.cross(n);

    float x_old, y_old;

    const double f = tRadius / tLenght;

    Vector3D v[4];
    v[0] = p + tRadius*d            + radius*n - radius*e;
    v[1] = p + (tRadius+2*radius)*d + radius*n - radius*e;
    v[2] = v[1] + tRadius*n;
    v[3] = v[0] + tRadius*n;
    glColor3f(colours[(side+3)%4].R(), colours[(side+3)%4].G(), colours[(side+3)%4].B());
    glNormal3d(-e[0], -e[1], -e[2]);
    glTexCoord2f (f, 0.2);
    glVertex3dv(&v[0][0]);
    glTexCoord2f (f, 0.2 + f * radius/0.2);
    glVertex3dv(&v[3][0]);
    glTexCoord2f (1-f, 0.2 + f * radius/0.2);
    glVertex3dv(&v[2][0]);
    glTexCoord2f (1-f, 0.2);
    glVertex3dv(&v[1][0]);

    v[0] = p + tRadius*d            + radius*n + radius*e;
    v[1] = p + (tRadius+2*radius)*d + radius*n + radius*e;
    v[2] = v[1] + tRadius*n;
    v[3] = v[0] + tRadius*n;
    glColor3f(colours[(side+1)%4].R(), colours[(side+1)%4].G(), colours[(side+1)%4].B());
    glNormal3d(e[0], e[1], e[2]);
    glTexCoord2f (f, 1);
    glVertex3dv(&v[0][0]);
    glTexCoord2f (1-f, 1);
    glVertex3dv(&v[1][0]);
    glTexCoord2f (1-f, 1 - f * radius/0.2);
    glVertex3dv(&v[2][0]);
    glTexCoord2f (f, 1 - f * radius/0.2);
    glVertex3dv(&v[3][0]);

    const size_t c_segs = 10;
    for (size_t c = 0; c < 2; c++) {
        if (c == 1) {
            p = p + tLenght * d;
            e = -e;
            d = -d;
        }

        for (size_t i = 0; i <= c_segs; i++) {
            float t = i / (double) c_segs;
            float a = t * M_PI * 0.5;
            float x = sin(a) * tRadius;
            float y = (1-cos(a)) * tRadius;

            l[0] = p + x*d + radius*n - radius*e;
            l[1] = p + x*d + radius*n + radius*e;
            s[0] = l[0] + y*n;
            s[1] = l[1] + y*n;

            if (i > 0) {

                double tex0 = (i-1) / (double) c_segs;
                double tex1 = i / (double) c_segs;

                if (lightAndTex)
                    glColor3f(colours[side].R(), colours[side].G(), colours[side].B());

                Vector3D normal = (q[0]-s[0]).cross(q[1]-q[0]);
                //normal.normalize(); // not needed

                glNormal3d(normal[0], normal[1], normal[2]);
                glTexCoord2f (tex1, 0.0);
                glVertex3dv(&s[0][0]);
                glTexCoord2f (tex0, 0.0);
                glVertex3dv(&q[0][0]);
                glTexCoord2f (tex0, 0.2);
                glVertex3dv(&q[1][0]);
                glTexCoord2f (tex1, 0.2);
                glVertex3dv(&s[1][0]);

                tex0 = x_old / tRadius * f;
                tex1 = x / tRadius * f;

                if (c == 1) {
                    tex0 = 1 - tex0;
                    tex1 = 1 - tex1;
                }

                if (lightAndTex)
                    glColor3f(colours[(side+3+c*2)%4].R(), colours[(side+3+c*2)%4].G(), colours[(side+3+c*2)%4].B());

                glNormal3d(-e[0], -e[1], -e[2]);
                if (c == 0) {
                    glTexCoord2f (tex0, 0.2 + 0.2*y_old/(2*radius));
                    glVertex3dv(&q[0][0]);
                    glTexCoord2f (tex1, 0.2 + 0.2*y/(2*radius));
                    glVertex3dv(&s[0][0]);
                    glTexCoord2f (tex1, 0.2);
                    glVertex3dv(&l[0][0]);
                    glTexCoord2f (tex0, 0.2);
                    glVertex3dv(&r[0][0]);
                } else {
                    glTexCoord2f (tex0, 1.0 - 0.2*y_old/(2*radius));
                    glVertex3dv(&q[0][0]);
                    glTexCoord2f (tex1, 1.0 - 0.2*y/(2*radius));
                    glVertex3dv(&s[0][0]);
                    glTexCoord2f (tex1, 1.0);
                    glVertex3dv(&l[0][0]);
                    glTexCoord2f (tex0, 1.0);
                    glVertex3dv(&r[0][0]);
                }

                if (lightAndTex)
                    glColor3f(colours[(side+1+c*2)%4].R(), colours[(side+1+c*2)%4].G(), colours[(side+1+c*2)%4].B());

                glNormal3dv(&e[0]);
                if (c == 0) {
                    glTexCoord2f (tex0, 1.0 - 0.2*y_old/(2*radius));
                    glVertex3dv(&q[1][0]);
                    glTexCoord2f (tex0, 1.0);
                    glVertex3dv(&r[1][0]);
                    glTexCoord2f (tex1, 1.0);
                    glVertex3dv(&l[1][0]);
                    glTexCoord2f (tex1, 1.0 - 0.2*y/(2*radius));
                    glVertex3dv(&s[1][0]);
                } else {
                    glTexCoord2f (tex0, 0.2 + 0.2*y_old/(2*radius));
                    glVertex3dv(&q[1][0]);
                    glTexCoord2f (tex0, 0.2);
                    glVertex3dv(&r[1][0]);
                    glTexCoord2f (tex1, 0.2);
                    glVertex3dv(&l[1][0]);
                    glTexCoord2f (tex1, 0.2 + 0.2*y/(2*radius));
                    glVertex3dv(&s[1][0]);
                }
            }

            q[0] = s[0];
            q[1] = s[1];

            r[0] = l[0];
            r[1] = l[1];

            x_old = x;
            y_old = y;
        }
    }
}

void Viewer::drawTBlock(const TSegment& t, bool lightAndTex) {

    Vector3D q[4];
    Vector3D l[4];

    Vector3D n = t.n(0);

    Vector3D e = t.d(0).cross(n);
    e.normalize();

    n = radius * n;
    e = radius * e;

    q[0] = t.p((size_t)0) + n + e;
    q[1] = t.p((size_t)0) - n + e;
    q[2] = t.p((size_t)0) - n - e;
    q[3] = t.p((size_t)0) + n - e;

    l[0] = t.p((size_t)1) + n + e;
    l[1] = t.p((size_t)1) - n + e;
    l[2] = t.p((size_t)1) - n - e;
    l[3] = t.p((size_t)1) + n - e;

    if (lightAndTex) {
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex[0]);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
    }

    glBegin(GL_QUADS);

    for (size_t i = 0; i < 4; i++) {

        if (t.side[i] == NULL) {
            if (lightAndTex)
                glColor3f(colours[i].R(), colours[i].G(), colours[i].B());

            switch (i) {
            case 1: glNormal3dv(&e[0]); break;
            case 2: glNormal3d(-n[0],-n[1],-n[2]); break;
            case 3: glNormal3d(-e[0],-e[1],-e[2]); break;
            case 0: glNormal3d(n[0],n[1],n[2]); break;
            }

            glTexCoord2f(1, 0.0);
            glVertex3dv(&l[(i+3)%4][0]);
            glTexCoord2f(0, 0.0);
            glVertex3dv(&q[(i+3)%4][0]);
            glTexCoord2f(0, 0.2);
            glVertex3dv(&q[i][0]);
            glTexCoord2f(1, 0.2);
            glVertex3dv(&l[i][0]);
        } else {
            Vector3D nn = t.n(0);
            nn.rotate(t.d(0), M_PI * 0.5 * i);
            drawT(t.p((size_t)0), t.d(0), nn, lightAndTex, i);
        }
    }

    glEnd();
}

void Viewer::drawLevel(bool lightAndTex) {
    Vector3D n_end;
    BOOST_FOREACH(Segment* segment, game->getLevel().segments) {
        switch (segment->getType()) {
        case BEZIER:
        case STRAIGHT:

            drawCurveBlock(*segment, lightAndTex);
            break;

        case T:
        {
            const TSegment* tSeg = static_cast<TSegment*>(segment);
            drawTBlock(*tSeg, lightAndTex);
            break;
        }

        default:
            assert(false);
            break;
        }
    }
}

void Viewer::on_realize() {
    // Do some OpenGL setup.
    // First, let the base class do whatever it needs to
    Gtk::GL::DrawingArea::on_realize();

    Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

    if (!gldrawable)
        return;

    if (!gldrawable->gl_begin(get_gl_context()))
        return;

    // Just enable depth testing and set the background colour.
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    //glShadeModel(GL_FLAT);
    glShadeModel(GL_SMOOTH);
    glPointSize(5);
    glEnable( GL_POINT_SMOOTH );

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gldrawable->gl_end();

    glewInit();

    createDrawBuffer();
    loadTextures();

    // load shaders
    list<string> defines;
    defines.push_back("VERTICAL_BLUR");
    defines.push_back("BLUR_BIG");
    shaderMgr.loadShader("glowVBig", "data/glow.vert", "data/glow.frag", defines);
    defines.clear();
    defines.push_back("HORIZONTAL_BLUR");
    defines.push_back("BLUR_BIG");
    shaderMgr.loadShader("glowHBig", "data/glow.vert", "data/glow.frag", defines);
    defines.clear();
    defines.push_back("VERTICAL_BLUR");
    defines.push_back("BLUR_SMALL");
    shaderMgr.loadShader("glowVSmall", "data/glow.vert", "data/glow.frag", defines);
    defines.clear();
    defines.push_back("HORIZONTAL_BLUR");
    defines.push_back("BLUR_SMALL");
    shaderMgr.loadShader("glowHSmall", "data/glow.vert", "data/glow.frag", defines);

    glActiveTexture(GL_TEXTURE0);

    shaderMgr.useShader("glowHBig");
    shaderMgr.setParam("tex", 0);
    shaderMgr.useShader("glowVBig");
    shaderMgr.setParam("tex", 0);
    shaderMgr.useShader("glowHSmall");
    shaderMgr.setParam("tex", 0);
    shaderMgr.useShader("glowVSmall");
    shaderMgr.setParam("tex", 0);

    shaderMgr.useShader(ShaderManager::defaultShader);

    qobj = gluNewQuadric();
    gluQuadricNormals(qobj, GLU_SMOOTH);
    gluQuadricDrawStyle(qobj, GLU_FILL);
    gluQuadricTexture(qobj, GL_TRUE);

    isGlInit = true;
}

void Viewer::createDrawBuffer() {

    for (size_t i = 0; i < 2; i++) {

        // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
        glGenFramebuffers(1, &fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);

        glGenTextures(1, &fboTex[i]);

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, fboTex[i]);

        // Give an empty image to OpenGL ( the last "0" )
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vboTexWidth, vboTexWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        // Poor filtering. Needed !
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTextureARB(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fboTex[i], 0);

        // Set the list of draw buffers.
        GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers

        // Always check that our framebuffer is ok
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw runtime_error("problem with the framebuffer");
        }

    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to screen
}

void Viewer::loadTextures() {
    const size_t nTex = sizeof(tex)/sizeof(tex[0]);
    glGenTextures(nTex, &tex[0]);

    for (size_t i = 0; i < nTex; i++) {

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, tex[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        size_t width = 512;
        if (i > 0 && i < 7)
            width = 1024;
        string filename;
        switch (i) {
        case 0: filename = "data/Rainbow Shards.data"; break;
        case 1: filename = "data/nightsky_north.data"; break;
        case 2: filename = "data/nightsky_west.data"; break;
        case 3: filename = "data/nightsky_south.data"; break;
        case 4: filename = "data/nightsky_east.data"; break;
        case 5: filename = "data/nightsky_up.data"; break;
        case 6: filename = "data/nightsky_down.data"; break;
        case 7: filename = "data/Optic Interconnect.data"; break;
        case 8: filename = "data/wheel-md.data"; width = 256; break;
        }

        ifstream file( filename.c_str(), ios::binary );
        if (!file) throw runtime_error("error reading file '" + filename + "'");
        // copies all data into buffer
        file.seekg (0, ios::end);
        size_t size = file.tellg();
        file.seekg (0, ios::beg);
        char* buffer = new char [size];
        file.read (buffer, size);
        file.close();
        if(size != width*width*3) {
            ostringstream err;
            err << "error reading texture '" << filename << "': size (" << size << ") != width (" << width << ") ^ 2 * 3";
            throw runtime_error(err.str().c_str());
        }

        // Textur wird hier in die Grafikkarte geladen! Dabei werden Mipmaps generiert.
        gluBuild2DMipmaps(GL_TEXTURE_2D,                    // 2D Textur wird geladen
                          GL_RGB,
                          width,     // Breite des Bildes
                          width,    // Höhe des Bildes
                          GL_RGB,                           // Format
                          GL_UNSIGNED_BYTE,                 // Wie Daten aufgeschriben sind
                          buffer);

        delete [] buffer;
    }
}

void Viewer::setProjAndModelViewMatrix() {

    // Set up perspective projection, using current size and aspect
    // ratio of display

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, (GLfloat) get_width() / (GLfloat) get_height(), 0.1, 1000.0);

    // Modify the current projection matrix so that we move the
    // camera away from the origin.  We'll draw the game at the
    // origin, and we need to back up to see it.

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /*glTranslated(0.0, 0.0, -7.0);
    glRotated(r, 1, 0.2, 0);*/

    const Segment* seg = game->getPlayerSeg();
    int side = game->getPlayerSide();
    double t = game->getPlayerT();

    Vector3D n = seg->n(t);
    Vector3D d = seg->d(t);
    n.rotate(d, side*M_PI*0.5);
    Vector3D p = seg->p(t);
    Vector3D g = p + 0.17 * n;

    Vector3D c = g + 5*camera;

    gluLookAt(c[0], c[1], c[2],
              g[0], g[1], g[2],
              n[0], n[1], n[2]);

}

void Viewer::scene(bool lightAndTex) {

    setProjAndModelViewMatrix();

    drawLevel(lightAndTex);

    if (enableParticleSystem && (!lightAndTex || glow == 0))
        particleSys->draw();
}

void Viewer::drawSkyBox() {

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    static double r = 0.0;
    r += 0.03;

    double c = 0.1*sin(r) + 0.8;

    glColor3f(c,c,c);

    const float d = 100.0f;
    const float t0 = 0.001;
    const float t1 = 0.999;

    glBindTexture(GL_TEXTURE_2D, tex[1]);

    glBegin(GL_QUADS);
    glTexCoord2f (t1, t1);
    glVertex3f(d, -d, -d);
    glTexCoord2f (t1, t0);
    glVertex3f(d, d, -d);
    glTexCoord2f (t0, t0);
    glVertex3f(-d, d, -d);
    glTexCoord2f (t0, t1);
    glVertex3f(-d, -d, -d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, tex[2]);

    glBegin(GL_QUADS);
    glTexCoord2f (t1, t1);
    glVertex3f(-d, -d, -d);
    glTexCoord2f (t1, t0);
    glVertex3f(-d, d, -d);
    glTexCoord2f (t0, t0);
    glVertex3f(-d, d, d);
    glTexCoord2f (t0, t1);
    glVertex3f(-d, -d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, tex[3]);

    glBegin(GL_QUADS);
    glTexCoord2f (t1, t1);
    glVertex3f(-d, -d, d);
    glTexCoord2f (t1, t0);
    glVertex3f(-d, d, d);
    glTexCoord2f (t0, t0);
    glVertex3f(d, d, d);
    glTexCoord2f (t0, t1);
    glVertex3f(d, -d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, tex[4]);

    glBegin(GL_QUADS);
    glTexCoord2f (t0, t1);
    glVertex3f(d, -d, -d);
    glTexCoord2f (t1, t1);
    glVertex3f(d, -d, d);
    glTexCoord2f (t1, t0);
    glVertex3f(d, d, d);
    glTexCoord2f (t0, t0);
    glVertex3f(d, d, -d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, tex[5]);

    glBegin(GL_QUADS);
    glTexCoord2f (t1, t0);
    glVertex3f(-d, d, -d);
    glTexCoord2f (t0, t0);
    glVertex3f(d, d, -d);
    glTexCoord2f (t0, t1);
    glVertex3f(d, d, d);
    glTexCoord2f (t1, t1);
    glVertex3f(-d, d, d);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, tex[6]);

    glBegin(GL_QUADS);
    glTexCoord2f (t1, t0);
    glVertex3f(-d, -d, -d);
    glTexCoord2f (t0, t0);
    glVertex3f(-d, -d, d);
    glTexCoord2f (t0, t1);
    glVertex3f(d, -d, d);
    glTexCoord2f (t1, t1);
    glVertex3f(d, -d, -d);
    glEnd();
}

void Viewer::drawRobot() {
    double r = 0.035;
    double l = 0.22;
    double d = 0.075;
    double x = 0.2;
    double y = 0.2;
    double h = 0.05;
    double r2 = 0.1;
    double r3 = 0.04;
    double r4 = 0.04*0.6;
    double l2 = 0.2;
    double slices = 16;
    double w = game->getPlayerWheel();

    glColor3f(1, 1, 1);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPushMatrix();

    glTranslated(-d, r, -l/2);
    for (size_t i = 0; i < 3; i++) {
        glColor3f(0.55, 0.5, 0.5);

        glBindTexture(GL_TEXTURE_2D, tex[7]);
        glPushMatrix();
        glRotated(w, 0, 0, -1);
        gluCylinder(qobj, r, r, l, slices, 1);
        gluQuadricOrientation(qobj, GLU_INSIDE);
        glColor3f(1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, tex[8]);
        gluDisk(qobj, 0, r, slices, 1);
        gluQuadricOrientation(qobj, GLU_OUTSIDE);
        glPushMatrix();
        glTranslated(0, 0, l);
        gluDisk(qobj, 0, r, slices, 1);
        glPopMatrix();
        glPopMatrix();
        glTranslated(d, 0, 0);

        glBindTexture(GL_TEXTURE_2D, tex[7]);
        if (i == 0) {
            glPushMatrix();
            glTranslated(0, r + r2, l/2);
            glColor3f(0.2, 0.5, 1);
            gluSphere(qobj, r2, slices, slices);

            glTranslated(0, -r2/2, 0);

            glColor3f(0.1, 0.1, 0.1);

            glPushMatrix();
            glRotatef(-30, 1, 0, 0);
            gluCylinder(qobj, r3, r4, l2, slices, 1);
            glPushMatrix();
            glTranslated(0, 0, l2);
            gluDisk(qobj, 0, r4, slices, slices);
            glPopMatrix();
            glPopMatrix();
            glPushMatrix();
            glRotatef(30 + 180, 1, 0, 0);
            gluCylinder(qobj, r3, r4, l2, slices, 1);
            glPushMatrix();
            glTranslated(0, 0, l2);
            gluDisk(qobj, 0, r4, slices, slices);
            glPopMatrix();
            glPopMatrix();

            glPopMatrix();

            glPushMatrix();
            glTranslated(-x/2, r, l/2-y/2);
            glColor3f(0.1, 0.1, 0.1);
            drawParallelepiped(Vector3D(), Vector3D(x, 0, 0), Vector3D(0, h, 0), Vector3D(0, 0, y));
            glPopMatrix();
        }
    }

    glPopMatrix();
}

bool Viewer::on_expose_event(GdkEventExpose*) {

    assert(isGlInit);
    Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

    if (!gldrawable)
        return false;

    if (!gldrawable->gl_begin(get_gl_context()))
        return false;

    // Clear the screen
    printOpenGLError();

    if (glow > 0) {

        /////////////// draw first pass of blur ////////////////

        glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_TEXTURE_2D);
        glViewport(0, 0, vboTexWidth, vboTexWidth);
        //glViewport(0, 0, get_width(), get_height());
        shaderMgr.useShader(ShaderManager::defaultShader);

        printOpenGLError();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene(false);

        printOpenGLError();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo[1]);
        glViewport(0, 0, vboTexWidth, vboTexWidth);
        //glViewport(0, 0, get_width(), get_height());

        printOpenGLError();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1, 0, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);

        /////////////// draw second pass of blur ////////////////

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        switch (glow) {
        //case 0: shaderMgr.useShader(ShaderManager::defaultShader); break;
        case 1: shaderMgr.useShader("glowHSmall"); break;
        case 2: shaderMgr.useShader("glowHBig"); break;
        }
        //glColor4f(1.0f,1.0f,0,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        printOpenGLError();

        glBindTexture(GL_TEXTURE_2D, fboTex[0]);
        glBegin(GL_QUADS);
        glTexCoord2f (0.0, 0.0);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glTexCoord2f (1.0, 0.0);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f (1.0, 1.0);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glTexCoord2f (0.0, 1.0);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glEnd();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, get_width(), get_height());

    /////////////// draw sky box to screen ////////////////

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    shaderMgr.useShader(ShaderManager::defaultShader);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setProjAndModelViewMatrix();

    if (enableSkyBox)
        drawSkyBox();
    //drawParallelepiped(Vector3D(0,0,-5), Vector3D(1,0,0), Vector3D(0,1,0), Vector3D(0,0,1));

    printOpenGLError();

    /////////////// draw blur to screen ////////////////

    if (glow > 0) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1, 0, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        switch (glow) {
        //case 0: shaderMgr.useShader(ShaderManager::defaultShader); break;
        case 1: shaderMgr.useShader("glowVSmall"); break;
        case 2: shaderMgr.useShader("glowVBig"); break;
        }
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        //glColor4f(1.0f,1.0f,0,1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, fboTex[1]);
        glBegin(GL_QUADS);
        glTexCoord2f (0.0, 0.0);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glTexCoord2f (1.0, 0.0);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f (1.0, 1.0);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glTexCoord2f (0.0, 1.0);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glEnd();
    }

    /////////////// draw actual scene to screen ////////////////

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    shaderMgr.useShader(ShaderManager::defaultShader);
    glClear(GL_DEPTH_BUFFER_BIT);

    scene(true);

    const Segment* seg = game->getPlayerSeg();
    int side = game->getPlayerSide();
    double t = game->getPlayerT();
    double tt = game->getPlayerTT();

    Vector3D n = seg->n(t);
    Vector3D d = seg->d(t);
    n.rotate(d, side*M_PI*0.5);
    Vector3D p = seg->p(t);
    Vector3D e = d.cross(n);
    Vector3D p0 = p + 0.14 * n /*- 0.1*d*/ + (tt /*- 0.1*/)*e;

    glPushMatrix();
    Matrix4x4 m (
            Vector4D(d[0], n[0], e[0], p0[0]),
            Vector4D(d[1], n[1], e[1], p0[1]),
            Vector4D(d[2], n[2], e[2], p0[2]),
            Vector4D(0, 0, 0, 1)
    );
    glMultMatrixd(m.transpose().begin());
    drawRobot();
    glPopMatrix();

    printOpenGLError();

    particleSys->update(0.05);

    printOpenGLError();

    glFlush();

    gldrawable->swap_buffers();

    gldrawable->gl_end();

    return true;
}

bool Viewer::on_configure_event(GdkEventConfigure* event) {
    Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();

    get_pointer(oldX, oldY);

    if (!gldrawable)
        return false;

    if (!gldrawable->gl_begin(get_gl_context()))
        return false;

    gldrawable->gl_end();

    return true;
}

bool Viewer::on_button_press_event(GdkEventButton* event) {
    oldX = event->x;
    oldY = event->y;
    return true;
}

bool Viewer::on_button_release_event(GdkEventButton*) {
    return true;
}

double sign(double x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

bool Viewer::on_motion_notify_event(GdkEventMotion* event) {
    int dx = event->x - oldX;
    int dy = event->y - oldY;
    Vector3D n = game->getPlayerSeg()->n(game->getPlayerT());
    Vector3D d = game->getPlayerSeg()->d(game->getPlayerT());
    n.rotate(d, M_PI * 0.5 * game->getPlayerSide());
    float speed = 0.01;
    camera.rotate(n, -dx*speed);
    Vector3D axis2 = n.cross(camera);
    axis2.normalize();
    if (camera.dot(n)*sign(-dy) > -0.99)
        camera.rotate(axis2, -dy*speed);
    camera.normalize();
    oldX = event->x;
    oldY = event->y;
    return true;
}
