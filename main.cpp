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

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <list>

using namespace std;

#define DEFAULT -1;

#define NEW_CURVE 0;

#define ADD_VERTEX 1;
#define MOVE_VERTEX 2;
#define REMOVE_VERTEX 3;

#define CHANGE_KNOTS 4;
#define CHANGE_K 5;

/*--------------------------- GL Global Variables ----------------------------*/
int grid_width;
int grid_height;

double pixel_size;

int win_height;
int win_width;

/*------------------- Helper Funct For Implementing Struct -------------------*/
int get_fact(int n) {
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result = result * i;
    }
    return result;
}

int get_nCr(int n, int r) {
    return get_fact(n) / (get_fact(r) * get_fact(n - r));
}

/*---------------------------- Data Structure Def ----------------------------*/
struct Vertex {
    double x,y;

    Vertex(double x1, double y1) {
        x = x1; y = y1;
    }
    Vertex(void) {
        x = 0; y = 0;
    }
};

struct Curve {
    vector<Vertex> vertices;
    
    /* bezier */
    int totalBezierVertices;
    vector<Vertex> bezierVertices;

    /* b-spline */
    int order_k;
    int degree;
    vector<double> knots;
    
    int totalBsplineVertices;
    vector<Vertex> bSplineVertices;
    
    Curve(void) {
        order_k = 3; degree = 3; totalBezierVertices = 100; totalBsplineVertices = 1000;
    }
    
    //0->n
    void bezier() {
        /* reset vector */
        bezierVertices.clear();
        
        /* get new vector */
        int totalVertices = (int)(vertices.size());
        for (int i = 0; i <= totalBezierVertices; i++) {
            Vertex bezierVertex = Vertex(0.0, 0.0);
            double t = double(i) / double(totalBezierVertices);
            int n = totalVertices - 1;
            for (int i = 0; i < totalVertices; i++) {
                bezierVertex.x += vertices.at(i).x * get_nCr(n, i) * pow(t, i) * pow(1-t, n-i);
                bezierVertex.y += vertices.at(i).y * get_nCr(n, i) * pow(t, i) * pow(1-t, n-i);
            }
            bezierVertices.push_back(bezierVertex);
        }
    }
    
    void bspline() {
        /* reset vectors */
        bSplineVertices.clear();
        knots.clear();
        
        /* get new knots vector */
        for (int i = 0; i < order_k + (int)vertices.size(); i++) {
            knots.push_back(i);
        }
        
        /* get new bspline vector */
        int k = order_k - 1;
        for (int i = 0; i < totalBsplineVertices; i++) {
            double x = (i/1000.0f) * (vertices.size() - k) + k;
            Vertex bsplineVertex = Vertex(0.0, 0.0);
            for (int j = 0; j < vertices.size(); j++) {
                bsplineVertex.x += vertices.at(j).x * deBoor(k, j, x);
                bsplineVertex.y += vertices.at(j).y * deBoor(k, j, x);
            }
            bSplineVertices.push_back(bsplineVertex);
        }
    }
    
    // from De Boor's algorithm's wikipedia page
    double deBoor(int k, int i, double x) {
        /* base case: k == 0 */
        if (k == 0) {
            if (knots.at(i) <= x && x < knots.at(i+1)) {
                return 1;
            } else {
                return 0;
            }
        }
        
        /* general case: k != 0 */
        return ((x - knots.at(i)) / (knots.at(i+k) - knots.at(i)) * deBoor(k-1, i, x)) + ((knots.at(i+k+1) - x) / (knots.at(i+k+1) - knots.at(i+1)) * deBoor(k-1, i+1, x));
    }
};

/*--------------------------- my Global Variables ----------------------------*/
vector<Curve> curves;
int mode = DEFAULT;
bool mouseinput = false;
bool mousedelete = false;
bool mouseadd = false;
bool mousechange = false;
bool isBezier = true;
int pindex = -1;
int id = -1;
int aid = -1;
int pid = -1;

/*--------------------------- my Helper Functions ----------------------------*/
void draw_input_line() {
    glBegin(GL_LINES);
    for (int i = 0; i < curves.size(); i++) {
        if (curves.at(i).vertices.size() >= 2) {
            for (int j = 1; j < curves.at(i).vertices.size(); j++) {
                glColor3f(0.0, 0.0, 1.0);
                glVertex2f(curves.at(i).vertices.at(j-1).x, curves.at(i).vertices.at(j-1).y);
                glVertex2f(curves.at(i).vertices.at(j).x, curves.at(i).vertices.at(j).y);
            }
        }
    }
    glEnd();
}
        
void draw_bezier() {
    glBegin(GL_LINES);
    for (int i = 0; i < curves.size(); i++) {
        if (curves.at(i).vertices.size() >= 3) {
            for (int j = 1; j < curves.at(i).bezierVertices.size(); j++) {
                glColor3f(1.0, 0.0, 0.0);
                glVertex2f(curves.at(i).bezierVertices.at(j-1).x, curves.at(i).bezierVertices.at(j-1).y);
                glVertex2f(curves.at(i).bezierVertices.at(j).x, curves.at(i).bezierVertices.at(j).y);
            }
        }
    }
    glEnd();
}
    
void draw_bspline() {
    glBegin(GL_LINES);
    for (int i = 0; i < curves.size(); i++) {
        if (curves.at(i).vertices.size() >= 3) {
            for (int j = 1; j < curves.at(i).bSplineVertices.size(); j++) {
                glColor3f(1.0, 0.0, 0.0);
                glVertex2f(curves.at(i).bSplineVertices.at(j-1).x, curves.at(i).bSplineVertices.at(j-1).y);
                glVertex2f(curves.at(i).bSplineVertices.at(j).x, curves.at(i).bSplineVertices.at(j).y);
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
    win_height = grid_height * pixel_size;
    win_width = grid_width * pixel_size;

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
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    /* draw input vertices */
    for (int i = 0; i < curves.size(); i++) {
        for (int j = 0; j < curves.at(i).vertices.size(); j++) {
            glPointSize(5);
            draw_pix(curves.at(i).vertices.at(j).x, curves.at(i).vertices.at(j).y);
        }
    }
    
    /* draw input lines */
    draw_input_line();
    
    /* draw bezier curves */
    if (isBezier == true) {
        for (int i = 0; i < curves.size(); i++) {
            curves.at(i).bezier();
        }
        draw_bezier();
    }
    
    /* draw b-spline curves */
    if (isBezier == false) {
        for (int i = 0; i < curves.size(); i++) {
            curves.at(i).bspline();
        }
        draw_bspline();
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

    pixel_size = width/(double)grid_width;

    glPointSize(pixel_size);
    check();
}

void key(unsigned char ch, int x, int y)
{
    Curve poly;
    Vertex temp;
    ofstream fout;
    fout.open("output.txt");
    switch(ch) {
        case 'b':
            isBezier = !isBezier;
            break;
        case 'i':
//            mouseinput = !mouseinput;
//            mouseadd = false;
//            mousechange = false;
//            mousedelete = false;
            mode = NEW_CURVE;
//            if (mouseinput) {
            cout << "Your new curve's id is: " << curves.size() << endl;
            id = (int)curves.size();
//            }
//            else {
//                id = -1;
//            }
            break;
        case 'a':
//            mouseadd = !mouseadd;
//            mouseinput = false;
//            mousechange = false;
//            mousedelete = false;
            mode = ADD_VERTEX;
//            if (mouseadd) {
            cout << "Current curve count: " << curves.size() << endl;
            cout << "please enter the curve id that you wang to add points" << endl;
            cin >> aid;
//            } else {
//                aid = -1;
//            }
            if (!(0 <= aid && aid < curves.size())) {
                cout << "invalid aid! (mode is set to DEFAULT)" << endl;
                mode = DEFAULT;
            }
            break;
        case 'p':
//            mousechange = !mousechange;
//            pindex = -1;
//            mouseinput = false;
//            mouseadd = false;
//            mousedelete = false;
            mode = MOVE_VERTEX;
//            if (mousechange) {
            cout << "Drag your desire vertex to new location on the screen" << endl;
//            }
            break;
        case 'd':
//            mousedelete = !mousedelete;
//            mouseadd = false;
//            mousechange = false;
//            mouseinput = false;
            mode = REMOVE_VERTEX;
            break;
        
        case 'c':
            for (int i = 0; i < curves.size(); i++) {
                curves.at(i).vertices.clear();
            }
            curves.clear();
            mode = DEFAULT;
//            curves.push_back(poly);
            break;
        case 'q':
            cout << "Thanks for your using :)" <<  endl;
            if (curves.size() == 1 && curves.at(0).vertices.at(0).x == -300) {
                fout << "No curve in this graph" << endl;
            }
            else {
                fout << "the number of the curve is: " << curves.size() << endl << endl;
                for (int i = 0; i < curves.size(); i++) {
                    fout << "the id of the curve is: " << i+1 << endl << endl;
                    fout << "the vertex of the curve is: " << endl;
                    for (int j = 0; j < curves.at(i).vertices.size(); j++) {
                        fout << "(" << curves.at(i).vertices.at(j).x << "," << curves.at(i).vertices.at(j).y << ")" << " --> ";
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

//    if (mouseinput) {
    if (mode == 0) { // mode == NEW_CURVE
        Vertex vertex = Vertex(newx, newy);
        /* case 1: creating a new curve */
        if (id == curves.size()) {
            Curve newCurve;
            newCurve.vertices.push_back(vertex);
            curves.push_back(newCurve);
        }
        /* case 2: adding new vertex in current curve */
        else {
            int verticesCount = (int)curves.at(id).vertices.size();
            // if no vertex, just push
            if (verticesCount == 0) {
                curves.at(id).vertices.push_back(vertex);
            // if there is vertex, check if too close to prev vertex
            } else {
                int lastx = curves.at(id).vertices.at(verticesCount - 1).x;
                int lasty = curves.at(id).vertices.at(verticesCount - 1).y;
                if (!(lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5)) {
                    curves.at(id).vertices.push_back(vertex);
                }
            }
        }
    }
    
//    if (mouseadd) {
    if (mode == 1) { // mode == ADD_VERTEX
        Vertex vertex = Vertex(newx, newy);
        int verticesCount = (int)curves.at(aid).vertices.size();
        // if no vertex, just push
        if (verticesCount == 0) {
            curves.at(id).vertices.push_back(vertex);
        // if there is vertex, check if too close to prev vertex
        } else {
            int lastx = curves.at(aid).vertices.at(verticesCount - 1).x;
            int lasty = curves.at(aid).vertices.at(verticesCount - 1).y;
            if (!(lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5)) {
                curves.at(aid).vertices.push_back(vertex);
            }
        }
    }
    
//    if (mousechange) {
    if (mode == 2) { //mode == MOVE_VERTEX
        /* case 1: to select a vertex */
        if (pindex < 0) {
            /* attempt to locate the target vertex */
            for (int i = 0; i < curves.size(); i++) {
                for (int j = 0; j < curves.at(i).vertices.size(); j++) {
                    int lastx = curves.at(i).vertices.at(j).x;
                    int lasty = curves.at(i).vertices.at(j).y;
                    if (lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5) {
                        pid = i;
                        pindex = j;
                        cout << "Selected vertex: curve " << pid << "'s vertex " << pindex << endl;
                    }
                }
            }
            /* if no vertex selected */
            if (pindex < 0) {
                cout << "Not a vertex. Try again" << endl;
            }
        /* case 2: move to desired position */
        } else {
            curves.at(pid).vertices.at(pindex).x = newx;
            curves.at(pid).vertices.at(pindex).y = newy;
            // reset pindex
            pindex = -1;
        }
    }

//    if (mousedelete) {
    if (mode == 3) { // mode == REMOVE_VERTEX
        /* attempt to locate the target vertex */
        for (int i = 0; i < curves.size(); i++) {
            for (int j = 0; j < curves.at(i).vertices.size(); j++) {
                int lastx = curves.at(i).vertices.at(j).x;
                int lasty = curves.at(i).vertices.at(j).y;
                if (lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5) {
                    curves.at(i).vertices.erase(curves.at(i).vertices.begin() + j);
                    cout << "Curve " << i << " has " << curves.at(i).vertices.size() << " vertices left" << endl;
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
