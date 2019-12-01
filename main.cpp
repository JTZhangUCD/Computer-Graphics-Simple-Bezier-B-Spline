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
    inline Vertex operator * (double n) const {
        return Vertex(x*n, y*n);
    }
    inline Vertex operator / (double n) const {
        return Vertex(x/n, y/n);
    }
};

struct Curve {                                                                // Curve
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
    

    void updateCurve() {
        int subCurveOrder = order_k; // = k = I want to break my curve into to cubics

        // De boor 1st attempt
        if(vertices.size() >= subCurveOrder) {
            knots.clear();
            createknots(subCurveOrder, (int)vertices.size());
            bSplineVertices.clear();

            for(int steps=0; steps<=1000; steps++) {
                // use steps to get a 0-1 range value for progression along the curve
                // then get that value into the range .at(k-1, n+1)
                // k-1 = subCurveOrder-1
                // n+1 = always the number of total control points
                double t = (steps / 1000.0f) * (vertices.size() - (subCurveOrder-1)) + subCurveOrder-1;
                Vertex temp;
                temp.x = temp.y = 0;
                for(int i=1; i <= vertices.size(); i++) {
                    double weightForControl = calculateWeightForPointI(i, subCurveOrder, (int)vertices.size(), t);
                    temp.x += weightForControl * vertices.at(i-1).x;
                    temp.y += weightForControl * vertices.at(i-1).y;
                }
                bSplineVertices.push_back(temp);
            }
        }
    }
    double calculateWeightForPointI(int i, int k, int cps, double t) {
        if (k == 1) {
            if (t >= knot(i) && t < knot(i+1))
                return 1;
            else
                return 0;
        }

        double numeratorA = (t - knot(i));
        double denominatorA = (knot(i + k-1) - knot(i));
        double numeratorB = (knot(i + k) - t);
        double denominatorB = (knot(i + k) - knot(i + 1));

        double subweightA = 0;
        double subweightB = 0;

        if (denominatorA != 0)
            subweightA = numeratorA / denominatorA * calculateWeightForPointI(i, k-1, cps, t);
        if (denominatorB != 0)
            subweightB = numeratorB / denominatorB * calculateWeightForPointI(i+1, k-1, cps, t);

        return subweightA + subweightB;
    }
    
    // from https://chi3x10.wordpress.com/2009/10/18/de-boor-algorithm-in-c/
    Vertex deBoor(int k, int i, double x) {
        if (k == 0) {
            return vertices.at(i);
        } else {
            double alpha = (x - knots.at(i)) / (knots.at(i + degree + 1 - k) - knots.at(i));
            return (deBoor(k-1, i-1, x)*(1-alpha) + deBoor(k-1, i, x)*alpha);
        }
    }
    
    double knot(int indexForKnot) {
        return knots.at(indexForKnot-1);
    }
    void createknots(int curveOrderK, int numControlPoints) {
        int knotSize = curveOrderK + numControlPoints;
        for(int count = 0; count <= knotSize; count++) {
            knots.push_back(count);
        }
    }
    
    void myBspline() {
        if (vertices.size() >= order_k) {
            /* get new knot vector */
            knots.clear();
            for (int i = 0; i <= order_k + (int)vertices.size(); i++) {
                knots.push_back(i);
            }
            
            /* get new spline vector */
            bSplineVertices.clear();
            for (int i = 0; i <= totalBsplineVertices; i++) {
                double x = (i / 1000.0f) * (vertices.size() - (order_k-1)) + order_k-1;
                Vertex bsplineVertex = Vertex(0.0, 0.0);
                for (int j = order_k; j < vertices.size(); j++) {
                    bsplineVertex.x += deBoor(order_k, j, x).x * vertices.at(j).x;
                    bsplineVertex.y += deBoor(order_k, j, x).y * vertices.at(j).y;
                }
                bSplineVertices.push_back(bsplineVertex);
            }
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
double kid = -1;
double kindex = -1;

/*--------------------------- my Helper Functions ----------------------------*/

void draw_input_line (Graph frame) {
    glBegin(GL_LINES);
    for (int i = 0; i < frame.curves.size(); i++) {
        if (frame.curves.at(i).vertices.size() >= 2) {
            for (int j = 1; j < frame.curves.at(i).vertices.size(); j++) {
                glColor3f(0.0, 0.0, 1.0);
                glVertex2f(frame.curves.at(i).vertices.at(j-1).x, frame.curves.at(i).vertices.at(j-1).y);
                glVertex2f(frame.curves.at(i).vertices.at(j).x, frame.curves.at(i).vertices.at(j).y);
            }
        }
    }
    glEnd();
}
        
void draw_bezier (Graph frame) {
    glBegin(GL_LINES);
    for (int i = 0; i < frame.curves.size(); i++) {
        if (frame.curves.at(i).vertices.size() >= 3) {
            for (int j = 1; j < frame.curves.at(i).bezierVertices.size(); j++) {
                glColor3f(1.0, 0.0, 0.0);
                glVertex2f(frame.curves.at(i).bezierVertices.at(j-1).x, frame.curves.at(i).bezierVertices.at(j-1).y);
                glVertex2f(frame.curves.at(i).bezierVertices.at(j).x, frame.curves.at(i).bezierVertices.at(j).y);
            }
        }
    }
    glEnd();
}
    
void draw_bspline (Graph frame) {
    glBegin(GL_LINES);
    for (int i = 0; i < frame.curves.size(); i++) {
        if (frame.curves.at(i).vertices.size() >= 3) {
            for (int j = 1; j < frame.curves.at(i).bSplineVertices.size(); j++) {
                glColor3f(1.0, 0.0, 0.0);
                glVertex2f(frame.curves.at(i).bSplineVertices.at(j-1).x, frame.curves.at(i).bSplineVertices.at(j-1).y);
                glVertex2f(frame.curves.at(i).bSplineVertices.at(j).x, frame.curves.at(i).bSplineVertices.at(j).y);
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
    for (int i = 0; i < origin.curves.size(); i++) {
        for (int j = 0; j < origin.curves.at(i).vertices.size(); j++) {
            glPointSize(5);
            draw_pix(origin.curves.at(i).vertices.at(j).x, origin.curves.at(i).vertices.at(j).y);
        }
    }
    
    /* draw input lines */
    draw_input_line(origin);
    
    /* draw bezier curves */
    if (isBezier == true) {
        for (int i = 0; i < origin.curves.size(); i++) {
            origin.curves.at(i).bezier();
        }
        draw_bezier(origin);
    }
    
    /* draw b-spline curves */
    if (isBezier == false) {
        for (int i = 0; i < origin.curves.size(); i++) {
            origin.curves.at(i).updateCurve();
//            origin.curves.at(i).myBspline();
        }
        draw_bspline(origin);
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
                    origin.curves.at(i).order_k = cur;
                }
            }
            else cout << "wrong curve number!!! :(" << endl;
            break;
        case 'b':
             isBezier = !isBezier;
             break;
        case 'i':
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
         case 'd':
             mousedelete = !mousedelete;
             mouseadd = false;
             mousechange = false;
             mouseinput = false;
             break;
         case 'a':
            mouseadd = !mouseadd;
            mouseinput = false;
            mousechange = false;
            mousedelete = false;
            if (mouseadd) {
                cout << "please enter the curve id that you wang to add points" << endl;
                cin >> aid;
            }
            else aid = -1;
            if (aid > origin.curves.size()) {
                cout << "curve id does not exist or add mode is turned off! :(" << endl;
            }
            break;
         case 'p':
             mousechange = !mousechange;
             pindex = -1;
             mouseinput = false;
             mouseadd = false;
             mousedelete = false;
             break;
         case 'c':
             for (int i = 0; i < origin.curves.size(); i++) {
                 origin.curves.at(i).vertices.clear();
             }
             origin.curves.clear();

             temp.x = -300;
             temp.y = -300;
             //vector <Vertex> v;
             //v.push_back(temp);

             poly.vertices.push_back(temp);
             origin.curves.push_back(poly);
             break;
          case 'q':
            cout << "Thanks for your using :)" <<  endl;
            if (origin.curves.size() == 1 && origin.curves.at(0).vertices.at(0).x == -300) {
                fout << "No curve in this graph" << endl;
            }
            else {
                fout << "the number of the curve is: " << origin.curves.size() << endl << endl;
                for (int i = 0; i < origin.curves.size(); i++) {
                    fout << "the id of the curve is: " << i+1 << endl << endl;
                    fout << "the vertex of the curve is: " << endl;
                for (int j = 0; j < origin.curves.at(i).vertices.size(); j++) {
                    fout << "(" << origin.curves.at(i).vertices.at(j).x << "," << origin.curves.at(i).vertices.at(j).y << ")" << " --> ";
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
            int l = (int)origin.curves.at(0).vertices.size();
            if (origin.curves.at(0).vertices.at(0).x != -300) {
            if (!((newx >= origin.curves.at(0).vertices.at(l-1).x-5 && newx <= origin.curves.at(0).vertices.at(l-1).x+5) && (newy >= origin.curves.at(0).vertices.at(l-1).y-5 && newy <= origin.curves.at(0).vertices.at(l-1).y+5))) {
                origin.curves.at(0).vertices.push_back(vertex);
                }
            }
                else {
                    origin.curves.at(0).vertices.at(0).x = newx;
                    origin.curves.at(0).vertices.at(0).y = newy;
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
                int l = (int)origin.curves.at(id-1).vertices.size();
                if (!((newx >= origin.curves.at(id-1).vertices.at(l-1).x-5 && newx <= origin.curves.at(id-1).vertices.at(l-1).x+5) && (newy >= origin.curves.at(id-1).vertices.at(l-1).y-5 && newy <= origin.curves.at(id-1).vertices.at(l-1).y+5))) {
                    origin.curves.at(id-1).vertices.push_back(v);
                }
            }
        }
    }

    if (mousedelete) {
        for (int j = 0; j < origin.curves.size(); j++) {
            for (int i = 0; i < origin.curves.at(j).vertices.size(); i++) {
                if ((newx >= origin.curves.at(j).vertices.at(i).x-5 && newx <= origin.curves.at(j).vertices.at(i).x+5) && (newy >= origin.curves.at(j).vertices.at(i).y-5 && newy <= origin.curves.at(j).vertices.at(i).y+5)) {
                    if (origin.curves.at(j).vertices.size() != 1)
                        origin.curves.at(j).vertices.erase(origin.curves.at(j).vertices.begin() + i);
                        else cout << "only one vertex left, please use clean to delete it" << endl;
                }
            }
        }
    }

    if (mouseadd && aid > 0) {
        bool exist = false;
        for (int i = 0; i < origin.curves.at(aid-1).vertices.size(); i++) {
            if ((newx >= origin.curves.at(aid-1).vertices.at(i).x-5 && newx <= origin.curves.at(aid-1).vertices.at(i).x+5) && (newy >= origin.curves.at(aid-1).vertices.at(i).y-5 && newy <= origin.curves.at(aid-1).vertices.at(i).y+5))
                exist = true;
        }

        if (exist) {
            //cout << "Sorry the point is already existed :(" << endl;
        }
        else {
            Vertex vertex;
            vertex.x = newx;
            vertex.y = newy;
            origin.curves.at(aid-1).vertices.push_back(vertex);
        }
    }

    if (mousechange && pindex >= 0) {
        origin.curves.at(pid).vertices.at(pindex).x = newx;
        origin.curves.at(pid).vertices.at(pindex).y = newy;
        //pindex = -1;
    }

    if (mousechange && pindex < 0) {
        for (int j = 0; j < origin.curves.size(); j++) {
            for (int i = 0; i < origin.curves.at(j).vertices.size(); i++) {
                if ((newx >= origin.curves.at(j).vertices.at(i).x-5 && newx <= origin.curves.at(j).vertices.at(i).x+5) && (newy >= origin.curves.at(j).vertices.at(i).y-5 && newy <= origin.curves.at(j).vertices.at(i).y+5)) {
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
