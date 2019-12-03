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

/*--------------------------- GL Global Variables ----------------------------*/
int grid_width;
int grid_height;

double pixel_size;

int win_height;
int win_width;

/* mode type            : index
        DEFAULT         :   -1
        NEW_CURVE       :   0
        ADD_VERTEX      :   1
        INSERT_VERTEX   :   2
        DELETE_VERTEX   :   3
        MODIFY_VERTEX   :   4
 */
int mode = -1;
bool isBezier = true;

/* for NEW_CURVE */
int curveID = -1;
/* for ADD_VERTEX */
int curveID_Add = -1;
/* for INSERT_VERTEX */
int curveID_Ins = -1;
int verticesIdx_Ins = -1;
/* for MODIFY_VERTEX */
int curveID_Mod = -1;
int verticesIdx_Mod = -1;
/* for CHANGE_ORDER_K */
int curveID_Odr = -1;
int newOrderK = -1;
/* for MODIFY_KNOTS */
int curveID_Knt = -1;
int knotIdx = -1;
double newKnotValue = -1.0;
bool updateKnot = false;

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
    int orderK;
    int degree;
    vector<double> knots;
    
    int totalBsplineVertices;
    vector<Vertex> bSplineVertices;
    
    Curve(void) {
        orderK = 3; totalBezierVertices = 1000; totalBsplineVertices = 1000;
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
        
        
        /* get new knots vector */
//        for (int i = 0; i < orderK + (int)vertices.size(); i++) {
//            knots.push_back(i);
//        }
        if (mode == 6) {
            knots.at(knotIdx) = newKnotValue;
        } else {
            knots.clear();
            for (int i = 0; i < orderK + (int)vertices.size(); i++) {
                knots.push_back(i);
            }
        }
        
        /* get new bspline vector */
        int k = orderK - 1;
        for (int i = 0; i < totalBsplineVertices; i++) {
            double x = (i/(double)totalBsplineVertices) * (vertices.size() - k) + k;
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
vector<Curve> curves;   // stores all curves

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
    ofstream fout;
    fout.open("output.txt");
    switch(ch)
    {
        case 's': {
            if (isBezier) {
                isBezier = false;
                cout << "Switched to B-spline Curve." << endl;
                
            } else {
                isBezier = true;
                cout << "Switched to Bezier Curve." << endl;
            }
            break;
        }
        case 'n': { //NEW_CURVE
            mode = 0;
            curveID = (int)curves.size();
            cout << "New curve created! Curve id: " << curveID << endl;
            break;
        }
        case 'a': { //ADD_VERTEX
            if (curves.size() == 0) {
                cout << "No available curve to add vertex." << endl;
            } else {
                mode = 1;
                cout << "*** ADD_VERTEX mode enabled! ***" << endl;
                cout << "Enter target curve id of ADD_VERTEX:";
                cout << "(Valid curve id: 0->" << curves.size()-1 << ")" << endl;
                cin >> curveID_Add;
                if (!(0 <= curveID_Add && curveID_Add < curves.size())) {
                    cout << "invalid curve id! (mode: DEFAULT)" << endl;
                    mode = -1;
                }
            }
            break;
        }
        case 'i': { //INSERT_VERTEX
            mode = 2;
            cout << "*** INSERT_VERTEX mode enabled! ***" << endl;
            cout << "Drag-create a new vertex from an old one." << endl;
            cout << "The new vertex will be inserted after the old one." << endl;
            break;
        }
        case 'd': { //DELETE_VERTEX
            mode = 3;
            cout << "*** DELETE_VERTEX mode enabled! ***" << endl;
            cout << "Click on the input vertex on screen to delete vertex." << endl;
            break;
        }
        case 'o': { //CHANGE_ORDER_K
            cout << "Enter your desire order of k:" << endl;
            if (curves.size() == 0) {
                cout << "No available curve to change order k." << endl;
            } else {
                /* get target curve id */
                cout << "Enter target curve id of CHANGE_ORDER_K:";
                cout << "(Valid curve id: 0->" << curves.size()-1 << ")" << endl;
                cin >> curveID_Odr;
                if (!(0 <= curveID_Odr && curveID_Odr < curves.size())) {
                    cout << "invalid curve id!" << endl;
                    break;
                }
                if (curves.at(curveID_Odr).vertices.size() == 0) {
                    cout << "invalid curve to change order k (no ctrl points)" << endl;
                    break;
                }
                
                /* get new order k */
                cout << "Enter the new order of k:" << endl;
                cin >> newOrderK;
                if (!(2 <= newOrderK && newOrderK <= curves.at(curveID_Odr).vertices.size())) {
                    cout << "invalid order of k (out of bound)" << endl;
                    break;
                }
                curves.at(curveID_Odr).orderK = newOrderK;
                cout << "order of k updated. Switch to B-spline to view result." << endl;
            }
            break;
        }
        case 'k': { //MODIFY_KNOTS
            mode = 6;
            cout << "*** MODIFY_KNOTS mode enabled! ***" << endl;
            cout << "Note: any modification will be disdarded if mode is changed." << endl;
            cout << "Note: must switch to B-spline first to have result." << endl;
            if (curves.size() == 0) {
                cout << "No available curve to modify knots." << endl;
            } else {
                int knotCount = (int)curves.at(0).knots.size();
                int shortestIdx = 0;
                for (int i = 1; i < curves.size(); i++) {
                    if (curves.at(i).knots.size() < knotCount) {
                        knotCount = (int)curves.at(i).knots.size();
                        shortestIdx = i;
                    }
                }
                
                /* print current knots vector */
                cout << "knots: {" << curves.at(shortestIdx).knots.at(0);
                for (int i = 1; i < curves.at(shortestIdx).knots.size(); i++) {
                    cout << "," << curves.at(shortestIdx).knots.at(i);
                }
                cout << "}" << endl;
                
                cout << "Enter the target knot index follow by new knot value" << endl;
                cin >> knotIdx >> newKnotValue;
                if (!(0 <= knotIdx && knotIdx < knotCount)) {
                    cout << "invalid knot index. (out of bound)" << endl;
                    break;
                }
                
                /* check if newKnotValue out of bound */
                double min = knotIdx == 0 ? 0.0 : curves.at(shortestIdx).knots.at(knotIdx-1);
                double max = knotIdx == knotCount-1 ? knotCount-1 : curves.at(shortestIdx).knots.at(knotIdx+1);
                if (!(min < newKnotValue && newKnotValue < max)) {
                    cout << "invalid new knot value. (out of bound)" << endl;
                    break;
                }
                
                cout << "knot updated. Switch to B-spline to view result." << endl;
            }
            break;
        }
        case 'm': { //MODIFY_VERTEX
            mode = 4;
            cout << "*** MODIFY_VERTEX mode enabled! ***" << endl;
            cout << "Drag your desire vertex to new location on screen." << endl;
            break;
        }
        case 'l': {                                                             //todo
            /* print current mode */
            switch (mode) {
                case 0:
                    cout << "Current mode: NEW_CURVE 0" << endl;
                    break;
                case 1:
                    cout << "Current mode: ADD_VERTEX 1" << endl;
                    break;
                case 2:
                    cout << "Current mode: INSERT_VERTEX 2" << endl;
                    break;
                case 3:
                    cout << "Current mode: DELETE_VERTEX 3" << endl;
                    break;
                case 4:
                    cout << "Current mode: MODIFY_VERTEX 4" << endl;
                    break;
                default:
                    break;
            }
            break;
        }
        case 'c': {
            curves.clear();
            mode = -1;
            cout << "*** All curves removed. mode: DEFAULT ***" << endl;
            break;
        }
        case 'q': {
            cout << "Thanks for your using :)" <<  endl;
            if (curves.size() == 0) {
                fout << "No curve in this graph" << endl;
            }
            else {
                fout << "the number of the curve is: " << curves.size() << endl << endl;
                for (int i = 0; i < curves.size(); i++) {
                    fout << "the id of the curve is: " << i << endl << endl;
                    fout << "the vertices of the curve is: " << endl;
                    for (int j = 0; j < curves.at(i).vertices.size(); j++) {
                        fout << "(" << curves.at(i).vertices.at(j).x << "," << curves.at(i).vertices.at(j).y << ")" << " --> ";
                    }
                    fout << "end" << endl << endl << endl;
                }
            }
            exit(0);
            break;
        }
        default: {
            //prints out which key the user hit
            printf("User hit the \"%c\" key\n",ch);
            break;
        }
    }

    glutPostRedisplay();
}


//gets called when a mouse button is pressed
void mouse(int button, int state, int x, int y)
{
    int newx = x;
    int newy = win_height-y;
    Vertex vertex = Vertex(newx, newy);
    
    switch(mode)
    {
        case 0: { //NEW_CURVE
            /* case 1: creating a new curve */
            if (curveID == curves.size()) {
                Curve newCurve;
                newCurve.vertices.push_back(vertex);
                curves.push_back(newCurve);
            /* case 2: adding new vertex in current curve */
            } else {
                int verticesCount = (int)curves.at(curveID).vertices.size();
                /* case 1: no vertex in curve */
                if (verticesCount == 0) {
                    curves.at(curveID).vertices.push_back(vertex);
                /* case 2: vertices in curve */
                } else {
                    int lastx = curves.at(curveID).vertices.at(verticesCount - 1).x;
                    int lasty = curves.at(curveID).vertices.at(verticesCount - 1).y;
                    if (!(lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5)) {
                        curves.at(curveID).vertices.push_back(vertex);
                    }
                }
            }
            break;
        }
        case 1: { //ADD_VERTEX
            int verticesCount = (int)curves.at(curveID_Add).vertices.size();
            /* case 1: no vertex in curve */
            if (verticesCount == 0) {
                curves.at(curveID_Add).vertices.push_back(vertex);
            /* case 2: vertices in curve */
            } else {
                int lastx = curves.at(curveID_Add).vertices.at(verticesCount - 1).x;
                int lasty = curves.at(curveID_Add).vertices.at(verticesCount - 1).y;
                if (!(lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5)) {
                    curves.at(curveID_Add).vertices.push_back(vertex);
                }
            }
            break;
        }
        case 2: { //INSERT_VERTEX
            if (verticesIdx_Ins == -1) {
                /* attempt to locate the target vertex */
                for (int i = 0; i < curves.size(); i++) {
                    for (int j = 0; j < curves.at(i).vertices.size(); j++) {
                        int lastx = curves.at(i).vertices.at(j).x;
                        int lasty = curves.at(i).vertices.at(j).y;
                        if (lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5) {
                            curveID_Ins = i;
                            verticesIdx_Ins = j;
//                            cout << "Selected vertex: curve " << curveID_Ins << "'s vertex " << verticesIdx_Ins << endl;
                        }
                    }
                }
                /* if no vertex selected */
                if (verticesIdx_Ins == -1) {
                    cout << "Not a vertex. Try again" << endl;
                }
            /* case 2: put new insert vertex to desired position */
            } else {
                curves.at(curveID_Ins).vertices.insert(curves.at(curveID_Ins).vertices.begin() + verticesIdx_Ins + 1, vertex);
                // reset verticesIdx_Ins
                verticesIdx_Ins = -1;
            }
            break;
        }
        case 3: { //DELETE_VERTEX
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
            break;
        }
        case 4: { //MODIFY_VERTEX
            /* case 1: to select a vertex */
            if (verticesIdx_Mod == -1) {
                /* attempt to locate the target vertex */
                for (int i = 0; i < curves.size(); i++) {
                    for (int j = 0; j < curves.at(i).vertices.size(); j++) {
                        int lastx = curves.at(i).vertices.at(j).x;
                        int lasty = curves.at(i).vertices.at(j).y;
                        if (lastx-5 <= newx && newx <= lastx+5 && lasty-5 <= newy && newy <= lasty+5) {
                            curveID_Mod = i;
                            verticesIdx_Mod = j;
//                            cout << "Selected vertex: curve " << curveID_Mod << "'s vertex " << verticesIdx_Mod << endl;
                        }
                    }
                }
                /* if no vertex selected */
                if (verticesIdx_Mod == -1) {
                    cout << "Not a vertex. Try again" << endl;
                }
            /* case 2: move to desired position */
            } else {
                curves.at(curveID_Mod).vertices.at(verticesIdx_Mod).x = newx;
                curves.at(curveID_Mod).vertices.at(verticesIdx_Mod).y = newy;
                // reset verticesIdx_Mod
                verticesIdx_Mod = -1;
            }
            break;
        }
        default: {
            cout << "mode without action" << endl;
            break;
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
