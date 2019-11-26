#ifdef WIN32
#include <windows.h>
#endif

#if defined (__APPLE__) || defined(MACOSX)
#include <OpenGL/gl.h>
//#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#else //linux
#include <GL/gl.h>
#include <GL/glut.h>
#endif

//other includes
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

/*--------------------------- GL Global Variables ----------------------------*/
int grid_width;
int grid_height;

float pixel_size;

int win_height;
int win_width;

/*---------------------------- Data Structure Def ----------------------------*/
struct Vertex {
    float x,y;

    Vertex(float x1, float y1) {
        x = x1; y = y1;
    }
    Vertex(void) {
        x = 0; y = 0;
    }

    inline Vertex operator = (const Vertex v1) {
        x = v1.x; y = v1.y;
        return *this;
    }
    inline Vertex operator + (const Vertex v1) const {
        return Vertex(x+v1.x, y+v1.y);
    }
    inline Vertex operator - (const Vertex v1) const {
        return Vertex(x-v1.x, y-v1.y);
    }
    inline Vertex operator * (float n) const {
        return Vertex(x*n, y*n);
    }
    inline Vertex operator / (float n) const {
        return Vertex(x/n, y/n);
    }
};

struct Curve {                                                                // Curve
    int curveID;
    vector<Vertex> vertices;
    
    /* bezier */
    int totalBezierVertices;
    vector<Vertex> bezierVertices;

    /* B-spline */
    int degree;
    bool k;
    vector<Vertex> bSplineVertices;
    vector<float> knotVector;
    Curve(void) {
        curveID = 3; degree = 3; totalBezierVertices = 100; k = false;
    }
    void binomialCoeffs(GLint n, GLint * C) {
        bool equal = false;
        bool jequal = false;
        GLint k, j;
        for (k = 0; k <= n; k++) {
            // Compute n!/(k!(n - k)!)
            C [k] = 1;
            for (j = n; j >= k + 1; j--) {
                if (j >= 2 && j <= n-k) {
                    equal = true;
                }
                if (equal == false) {
                    C[k] *= j;
                }
                else {equal = false;}
            }
            for (j = n - k; j >= 2; j--) {
                if (j >= k+1 && j <= n) {
                    jequal = true;
                }
                if (jequal == false) {
                    C [k] /= j;
                } else {
                    jequal = false;
                }
            }
        }
    }
    void getBezierVertex(GLfloat t, Vertex *bezierVertex, GLint *nCrList, int totalVertices) {
        GLint n = totalVertices - 1;
        bezierVertex->x = 0.0; // reset to zero
        bezierVertex->y = 0.0;
        for (int i = 0; i < totalVertices; i++) {
            bezierVertex->x += vertices[i].x * nCrList[i] * pow(t, i) * pow(1-t, n-i);
            bezierVertex->y += vertices[i].y * nCrList[i] * pow(t, i) * pow(1-t, n-i);
        }
    }
    void bezier() {
        Vertex bezierVertex;
        GLfloat t;
    
        int totalVertices = (int)(vertices.size());
        GLint *nCrList = new GLint [totalVertices+1];
        binomialCoeffs(totalVertices - 1, nCrList);

        for (int i = 0; i <= totalBezierVertices; i++) {
            t = GLfloat(i) / GLfloat(totalBezierVertices);
            getBezierVertex(t, &bezierVertex, nCrList, totalVertices);
            bezierVertices.push_back(bezierVertex);
        }
    }
    void updateCurve() {
        int subCurveOrder = curveID; // = k = I want to break my curve into to cubics

        // De boor 1st attempt
        if(vertices.size() >= subCurveOrder) {
            knotVector.clear();
            createKnotVector(subCurveOrder, (int)vertices.size());
            bSplineVertices.clear();

            for(int steps=0; steps<=1000; steps++) {
                // use steps to get a 0-1 range value for progression along the curve
                // then get that value into the range [k-1, n+1]
                // k-1 = subCurveOrder-1
                // n+1 = always the number of total control points

                float t = (steps / 1000.0f) * ( vertices.size() - (subCurveOrder-1) ) + subCurveOrder-1;

                Vertex temp;
                temp.x = temp.y = 0;
                for(int i=1; i <= vertices.size(); i++) {
                    float weightForControl = calculateWeightForPointI(i, subCurveOrder, (int)vertices.size(), t);
                    temp.x += weightForControl * vertices[i-1].x;
                    temp.y += weightForControl * vertices[i-1].y;
                }
                bSplineVertices.push_back(temp);
            }
        }
    }
    float calculateWeightForPointI(int i, int k, int cps, float t) {
        if(k == 1) {
            if(t >= knot(i) && t < knot(i+1))
                return 1;
            else
                return 0;
        }

        float numeratorA = ( t - knot(i) );
        float denominatorA = ( knot(i + k-1) - knot(i) );
        float numeratorB = ( knot(i + k) - t );
        float denominatorB = ( knot(i + k) - knot(i + 1) );

        float subweightA = 0;
        float subweightB = 0;

        if( denominatorA != 0 )
            subweightA = numeratorA / denominatorA * calculateWeightForPointI(i, k-1, cps, t);
        if( denominatorB != 0 )
            subweightB = numeratorB / denominatorB * calculateWeightForPointI(i+1, k-1, cps, t);

        return subweightA + subweightB;
    }
    float knot(int indexForKnot) {
        return knotVector.at(indexForKnot-1);
    }
    void createKnotVector(int curveOrderK, int numControlPoints) {
        int knotSize = curveOrderK + numControlPoints;
        for(int count = 0; count <= knotSize; count++) {
                knotVector.push_back( count );
        }
    }
};

struct Graph {                                                                  // Graph
    vector<Curve> curves;

    Graph() {
        Vertex temp;
        Curve curve;
        temp.x = -300;
        temp.y = -300;

        curve.vertices.push_back(temp);
        curves.push_back(curve);
    }
};

/*--------------------------- my Global Variables ----------------------------*/
Graph origin;
bool mouseinput = false;
bool mousedelete = false;
bool mouseadd = false;
bool mousechange = false;
bool isBezier = true;
int pindex = -1;
int id = -1;
int aid = -1;
int pid = -1;
float kid = -1;
float kindex = -1;

/*--------------------------- my Helper Functions ----------------------------*/
void draw_lines(Graph frame, bool isline){
    glLineWidth(2.0); //sets the "width" of each line we are rendering
    //tells opengl to interperate every two calls to glVertex as a line
    glBegin(GL_LINES);
    //first line will be blue
    if (!isline) {
        for (int i = 0; i < frame.curves.size(); i++) {
            if (frame.curves[i].vertices.size() > 1) {
                for (int j = 0; j < frame.curves[i].vertices.size() - 1; j++) {
                glColor3f(0, 0, 1);
                glVertex2f(frame.curves[i].vertices[j].x, frame.curves[i].vertices[j].y);
                glVertex2f(frame.curves[i].vertices[j+1].x, frame.curves[i].vertices[j+1].y);
                }
            }
        }
    }
    else {
        for (int i = 0; i < frame.curves.size(); i++) {
            if (!isBezier && frame.curves[i].bSplineVertices.size() != 0) {
                if (frame.curves[i].vertices.size() > 2) {
                    for (int j = 0; j < frame.curves[i].bSplineVertices.size() - 1; j++) {
                        glColor3f(1, 0, 0);
                        glVertex2f(frame.curves[i].bSplineVertices[j].x, frame.curves[i].bSplineVertices[j].y);
                        glVertex2f(frame.curves[i].bSplineVertices[j+1].x, frame.curves[i].bSplineVertices[j+1].y);
                    }
                }
            }
        }

        for (int i = 0; i < frame.curves.size(); i++) {
            if (isBezier && frame.curves[i].bezierVertices.size() != 0) {
                if (frame.curves[i].vertices.size() > 2) {
                    for (int j = 0; j < frame.curves[i].bezierVertices.size() - 1; j++) {
                        glColor3f(1, 0, 0);
                        glVertex2f(frame.curves[i].bezierVertices[j].x, frame.curves[i].bezierVertices[j].y);
                        glVertex2f(frame.curves[i].bezierVertices[j+1].x, frame.curves[i].bezierVertices[j+1].y);
                    }
                }
            }
        }
    }
    glEnd();
}

/*--------------------------- GL Helper Functions ----------------------------*/
void init();
void idle();
void display();
void draw_pix(int x, int y);
void reshape(int width, int height);
void key(unsigned char ch, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void check();

/*----------------------------- Main Functions -------------------------------*/
int main(int argc, char **argv)
{
    grid_width = 800;
    grid_height = 800;
    
    pixel_size = 1;

    win_height = grid_height*pixel_size;
    win_width = grid_width*pixel_size;

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(win_width,win_height);
    glutCreateWindow("glut demo");

    /*defined glut callback functions*/
    glutDisplayFunc(display); //rendering calls here
    glutReshapeFunc(reshape); //update GL on window size change
    glutMouseFunc(mouse);     //mouse button events
    glutMotionFunc(motion);   //mouse movement events
    glutKeyboardFunc(key);    //Keyboard events
    glutIdleFunc(idle);       //Function called while program is sitting "idle"

    init();
    glutMainLoop();
    return 0;
}

void init()
{
    glClearColor(0,0,0,0);

    cout << "The default display mode is Bezier Curve\n";
    cout << "If you want to change the curve method,\n";
    cout << "please press B on the graph window and follow the instructions:)\n" << endl;

    check();
}

void idle()
{
    glutPostRedisplay();
}

void display()
{
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    if (origin.curves.size() != 0) {
        draw_lines(origin, false);

        for (int i = 0; i < origin.curves.size(); i++) {
            for (int j = 0; j < origin.curves[i].vertices.size(); j++) {
                glPointSize(5);
                draw_pix(origin.curves[i].vertices[j].x , origin.curves[i].vertices[j].y);
            }
            if (isBezier) {
                if (origin.curves[i].vertices.size() > 1) {
                    origin.curves[i].bezierVertices.clear();
                    origin.curves[i].bezier();
                }
                if (origin.curves[i].vertices.size() > 1)
                     draw_lines(origin, true);
            } else {
                origin.curves[i].updateCurve();
                if (origin.curves[i].vertices.size() > 1)
                    draw_lines(origin, true);
            }
        }
    }

    glutSwapBuffers();
    check();
}

void draw_pix(int x, int y){
    glBegin(GL_POINTS);
    glColor3f(1.0,0,0);
    glVertex3f(x,y,0);
    glEnd();
}

void reshape(int width, int height)
{
    win_width = width;
    win_height = height;

    glViewport(0,0,width,height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,grid_width,0,grid_height,-10,10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    pixel_size = width/(float)grid_width;

    glPointSize(pixel_size);
    check();
}

void key(unsigned char ch, int x, int y)
{
    Curve poly;
    Vertex temp;
    int cur;
    ofstream fout;
    fout.open("output.txt");
    switch(ch)
    {
       case 'u':
         mouseinput = false;
         mouseadd = false;
         mousechange = false;
         mousedelete = false;
         cout << "please enter new sub curve number:" << endl;
         cin >> cur;
         if (cur > 1 && cur < 5) {
             for (int i = 0; i < origin.curves.size(); i++) {
                 origin.curves[i].curveID = cur;
             }
         }
         else cout << "wrong curve number!!! :(" << endl;
         break;
        case 'b' :
             isBezier = !isBezier;
             break;
        case 'i' :
         mouseinput = !mouseinput;
         mouseadd = false;
         mousechange = false;
         mousedelete = false;
         if (mouseinput) {
             cout << "please enter the new curve id" << endl;
             cin >> id;
         }
         else id = -1;
         break;
         case 'd' :
             mousedelete = !mousedelete;
             mouseadd = false;
             mousechange = false;
             mouseinput = false;
             break;
         case 'a' :
             mouseadd = !mouseadd;
             mouseinput = false;
             mousechange = false;
             mousedelete = false;
             if (mouseadd) {
                 cout << "please enter the curve id that you wang to add points" << endl;
                 cin >> aid;
             }
             else aid = -1;
             if (aid > origin.curves.size())
             {
               cout << "curve id does not exist or add mode is turned off! :(" << endl;
             }
             break;
         case 'p' :
             mousechange = !mousechange;
             pindex = -1;
             mouseinput = false;
             mouseadd = false;
             mousedelete = false;
             break;
         case 'c' :
             for (int i = 0; i < origin.curves.size(); i++) {
                 origin.curves[i].vertices.clear();
             }
             origin.curves.clear();

             temp.x = -300;
             temp.y = -300;
             //vector <Vertex> v;
             //v.push_back(temp);

             poly.vertices.push_back(temp);
             origin.curves.push_back(poly);
             break;
          case 'q' :
          cout << "Thanks for your using :)" <<  endl;
          if (origin.curves.size() == 1 && origin.curves[0].vertices[0].x == -300) {
           fout << "No curve in this graph" << endl;
          }
          else {
            fout << "the number of the curve is: " << origin.curves.size() << endl << endl;
            for (int i = 0; i < origin.curves.size(); i++) {
             fout << "the id of the curve is: " << i+1 << endl << endl;
             fout << "the vertex of the curve is: " << endl;
             for (int j = 0; j < origin.curves[i].vertices.size(); j++) {
               fout << "(" << origin.curves[i].vertices[j].x << "," << origin.curves[i].vertices[j].y << ")" << " --> ";
             }
             fout << "end" << endl << endl << endl;
            }
          }
          exit(0);
          break;
        default:
            //prints out which key the user hit
            printf("User hit the \"%c\" key\n",ch);
            break;
    }

    glutPostRedisplay();
}


//gets called when a mouse button is pressed
void mouse(int button, int state, int x, int y)
{
    int newx = (int)(x);
    int newy = (int)((win_height-y));
//    int tid;

    if (mouseinput && id > 0){
        Vertex vertex;
        vertex.x = newx;
        vertex.y = newy;
        if (id == 1) {
            int l = (int)origin.curves[0].vertices.size();
            if (origin.curves[0].vertices[0].x != -300) {
            if (!((newx >= origin.curves[0].vertices[l-1].x-5 && newx <= origin.curves[0].vertices[l-1].x+5) && (newy >= origin.curves[0].vertices[l-1].y-5 && newy <= origin.curves[0].vertices[l-1].y+5))) {
                origin.curves[0].vertices.push_back(vertex);
                }
            }
                else {
                    origin.curves[0].vertices[0].x = newx;
                    origin.curves[0].vertices[0].y = newy;
                }
        }
        else if (id > 1) {
            if (id > origin.curves.size()) {

                Vertex v;
                Curve poly;
                v.x = newx;
                v.y = newy;
                poly.vertices.push_back(v);
                origin.curves.push_back(poly);
            }
            else {
                Vertex v;
                v.x = newx;
                v.y = newy;
                int l = (int)origin.curves[id-1].vertices.size();
                if (!((newx >= origin.curves[id-1].vertices[l-1].x-5 && newx <= origin.curves[id-1].vertices[l-1].x+5) && (newy >= origin.curves[id-1].vertices[l-1].y-5 && newy <= origin.curves[id-1].vertices[l-1].y+5))) {
                    origin.curves[id-1].vertices.push_back(v);
                }
            }
        }
    }

    if (mousedelete) {
        for (int j = 0; j < origin.curves.size(); j++) {
            for (int i = 0; i < origin.curves[j].vertices.size(); i++) {
                if ((newx >= origin.curves[j].vertices[i].x-5 && newx <= origin.curves[j].vertices[i].x+5) && (newy >= origin.curves[j].vertices[i].y-5 && newy <= origin.curves[j].vertices[i].y+5)) {
                    if (origin.curves[j].vertices.size() != 1)
                        origin.curves[j].vertices.erase(origin.curves[j].vertices.begin() + i);
                        else cout << "only one vertex left, please use clean to delete it" << endl;
                }
            }
        }
    }

    if (mouseadd && aid > 0) {
        bool exist = false;
        for (int i = 0; i < origin.curves[aid-1].vertices.size(); i++) {
            if ((newx >= origin.curves[aid-1].vertices[i].x-5 && newx <= origin.curves[aid-1].vertices[i].x+5) && (newy >= origin.curves[aid-1].vertices[i].y-5 && newy <= origin.curves[aid-1].vertices[i].y+5))
                exist = true;
        }

        if (exist) {
            //cout << "Sorry the point is already existed :(" << endl;
        }
        else {
            Vertex vertex;
            vertex.x = newx;
            vertex.y = newy;
            origin.curves[aid-1].vertices.push_back(vertex);
        }
    }

    if (mousechange && pindex >= 0) {
        origin.curves[pid].vertices[pindex].x = newx;
        origin.curves[pid].vertices[pindex].y = newy;
        //pindex = -1;
    }

    if (mousechange && pindex < 0) {
        for (int j = 0; j < origin.curves.size(); j++) {
            for (int i = 0; i < origin.curves[j].vertices.size(); i++) {
                if ((newx >= origin.curves[j].vertices[i].x-5 && newx <= origin.curves[j].vertices[i].x+5) && (newy >= origin.curves[j].vertices[i].y-5 && newy <= origin.curves[j].vertices[i].y+5)) {
                    pindex = i;
                    pid = j;
                }
            }
        }
    }
    //redraw the scene after mouse click
    glutPostRedisplay();
}

void motion(int x, int y)
{
    glutPostRedisplay();
}


void check()
{
    GLenum err = glGetError();
    if(err != GL_NO_ERROR)
    {
        printf("GLERROR: There was an error %s\n",gluErrorString(err) );
        exit(1);
    }
}
