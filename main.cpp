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

double pixel_size;

int win_height;
int win_width;

/*---------------------------- Data Structure Def ----------------------------*/
struct Vertex {                                                                 // Vertex
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
    inline Vertex operator * (double m) const {
        return Vertex(x*m, y*m);
    }
    inline Vertex operator / (double m) const {
        return Vertex(x+m, y+m);
    }
};


struct Polygon {                                                                // Polygon
    int curve;
    int degree;
    /* bezier */
    int nBezCurvePts;
    vector<Vertex> vertices;
    vector<Vertex> bezPoints;
    
    /* B-spline */
    bool k;
    vector<double> knots;
    vector<Vertex> splPoints;
    vector<double> knotVector;
    
    vector<int> temp;
    
    Polygon(void) {
        curve = 3; degree = 3; nBezCurvePts = 100; k = false;
    }
    void binomialCoeffs(GLint n, GLint * C) {
        bool equal = false;
        bool jequal = false;
//        int e;
        GLint k, j;
        for (k = 0; k <= n; k++) {
            // Compute n!/(k!(n - k)!)
            C [k] = 1;
            for (j = n; j >= k + 1; j--) {
                if (j >= 2 && j <= n-k) {
                    equal = true;
                }
                if (equal == false) {
                    C [k] *= j;
                }
                else {equal = false;}
            }
             for (j = n - k; j >= 2; j--) {
                if (j >= k+1 && j <= n) {
                     jequal = true;
                }
                 if (jequal == false) {
                    C [k] /= j;
                 }
                 else {jequal = false;}
             }
        }
    }
    void computeBezPt(GLdouble u, Vertex *bezPt, GLint *C, int size) {
        int nCtrlPts = size;
        GLint k, n = nCtrlPts - 1;
        GLdouble bezBlendFcn;
        bezPt->x = 0.0;
        bezPt->y = 0.0;

        for (k = 0; k < nCtrlPts; k++) {
            bezBlendFcn = C [k] * pow (u, k) * pow (1 - u, n - k);
            bezPt->x += vertices [k].x * bezBlendFcn;
            bezPt->y += vertices [k].y * bezBlendFcn;
        }
    }
    void bezier() {
        temp.clear();
        Vertex bezCurvePt;
        GLdouble u;
        GLint *C, k;
        
        int nCtrlPts = (int)(vertices.size());

        // Allocate space for binomial coefficients
        C = new GLint [nCtrlPts+1];

        binomialCoeffs(nCtrlPts - 1, C);

        for (k = 0; k <= nBezCurvePts; k++) {
            u = GLdouble (k) / GLdouble (nBezCurvePts);
            computeBezPt (u, &bezCurvePt, C, nCtrlPts);
            // plotPoint (bezCurvePt);
            bezPoints.push_back(bezCurvePt);
        }
    }
    void changeDegree (int val) {
        degree = val;
    }
    void initKnots() {
        for(int i = 0; i < degree; i++)
            knots.push_back(0);

        int nSegment = (int)(vertices.size() - degree);
        
        if(nSegment == 0) {
            knots.push_back(0);
        } else {
            for(int i=0; i <= nSegment; i++)
                knots.push_back(((double) i)/nSegment);
        }

        for(int i=0; i < degree; i++)
            knots.push_back(1);
    }
    void updateCurve() {
        int subCurveOrder = curve; // = k = I want to break my curve into to cubics

        // De boor 1st attempt
        if(vertices.size() >= subCurveOrder) {
            knotVector.clear();
            createKnotVector(subCurveOrder, (int)vertices.size());
            splPoints.clear();

            for(int steps=0; steps<=1000; steps++) {
                // use steps to get a 0-1 range value for progression along the curve
                // then get that value into the range [k-1, n+1]
                // k-1 = subCurveOrder-1
                // n+1 = always the number of total control points

                double t = ( steps / 1000.0f ) * ( vertices.size() - (subCurveOrder-1) ) + subCurveOrder-1;

                Vertex temp;
                temp.x = temp.y = 0;
                for(int i=1; i <= vertices.size(); i++) {
                    double weightForControl = calculateWeightForPointI(i, subCurveOrder, (int)vertices.size(), t);
                    temp.x += weightForControl * vertices[i-1].x;
                    temp.y += weightForControl * vertices[i-1].y;
                }
                splPoints.push_back(temp);
            }
        }
    }
    double calculateWeightForPointI(int i, int k, int cps, double t) {
        if(k == 1) {
            if(t >= knot(i) && t < knot(i+1))
                return 1;
            else
                return 0;
        }

        double numeratorA = ( t - knot(i) );
        double denominatorA = ( knot(i + k-1) - knot(i) );
        double numeratorB = ( knot(i + k) - t );
        double denominatorB = ( knot(i + k) - knot(i + 1) );

        double subweightA = 0;
        double subweightB = 0;

        if( denominatorA != 0 )
            subweightA = numeratorA / denominatorA * calculateWeightForPointI(i, k-1, cps, t);
        if( denominatorB != 0 )
            subweightB = numeratorB / denominatorB * calculateWeightForPointI(i+1, k-1, cps, t);

        return subweightA + subweightB;
    }
    double knot(int indexForKnot) {
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
    vector<Polygon> polys;
    int xMax, xMin, yMax, yMin, zMax, zMin;
    
    Graph() {};
    void initialization() {
        Vertex temp;
        Polygon poly;
        temp.x = -300;
        temp.y = -300;
        
        poly.vertices.push_back(temp);
        polys.push_back(poly);
    }
    void printInfo(int i) {
        i--;
        for (int j = 0; j < polys[i].vertices.size(); j++) {
            cout << polys[i].vertices[j].x << ' ' << polys[i].vertices[j].y << endl;
        }
        cout << xMin << ' ' << xMax << ' ' << yMin <<  ' ' << yMax << endl;
    }
    void setXYZ() {
        for (int i = 0; i < polys.size(); i++) {
            for (int j = 0; j < polys[i].vertices.size(); j++) {
                int x;
                if (polys[i].vertices[j].x > 0)
                    x = ceil(polys[i].vertices[j].x);
                else
                    x = floor(polys[i].vertices[j].x);

                if (polys[i].vertices[j].x > xMax)
                    xMax = x;
                if (polys[i].vertices[j].x < xMin)
                    xMin = x;

                if (polys[i].vertices[j].y > 0)
                    x = ceil(polys[i].vertices[j].y);
                else
                    x = floor(polys[i].vertices[j].y);
                if (polys[i].vertices[j].y > yMax)
                    yMax = x;
                if (polys[i].vertices[j].y < yMin)
                    yMin = x;
            }
        }

        cout << "xMin: " << xMin << endl;
        cout << "xMax: " << xMax << endl;
        cout << "yMin: " << yMin << endl;
        cout << "yMax: " << yMax << endl;
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
void draw_lines(Graph frame, bool isline){
   glLineWidth(2.0); //sets the "width" of each line we are rendering
   //tells opengl to interperate every two calls to glVertex as a line
   glBegin(GL_LINES);
   //first line will be blue

    if (!isline) {
        for (int i = 0; i < frame.polys.size(); i++) {
            if (frame.polys[i].vertices.size() > 1) {
                for (int j = 0; j < frame.polys[i].vertices.size() - 1; j++) {
                glColor3f(0, 0, 1);
                glVertex2f(frame.polys[i].vertices[j].x, frame.polys[i].vertices[j].y);
                glVertex2f(frame.polys[i].vertices[j+1].x, frame.polys[i].vertices[j+1].y);
                }
            }
        }
    }
    else {
        
            for (int i = 0; i < frame.polys.size(); i++) {
                if (!isBezier && frame.polys[i].splPoints.size() != 0) {
                if (frame.polys[i].vertices.size() > 2) {
                for (int j = 0; j < frame.polys[i].splPoints.size() - 1; j++) {
                    glColor3f(1, 0, 0);
                    glVertex2f(frame.polys[i].splPoints[j].x, frame.polys[i].splPoints[j].y);
                    glVertex2f(frame.polys[i].splPoints[j+1].x, frame.polys[i].splPoints[j+1].y);
                }
                }
            }
        }

            for (int i = 0; i < frame.polys.size(); i++) {
                if (isBezier && frame.polys[i].bezPoints.size() != 0) {
                if (frame.polys[i].vertices.size() > 2) {
                for (int j = 0; j < frame.polys[i].bezPoints.size() - 1; j++) {
                    glColor3f(1, 0, 0);
                    glVertex2f(frame.polys[i].bezPoints[j].x, frame.polys[i].bezPoints[j].y);
                    glVertex2f(frame.polys[i].bezPoints[j+1].x, frame.polys[i].bezPoints[j+1].y);
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
void key(unsigned char ch, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void check();

/*----------------------------- Main Functions -------------------------------*/
int main(int argc, char **argv)
{
    
    //the number of pixels in the grid
    grid_width = 800;
    grid_height = 800;
    
    //the size of pixels sets the inital window height and width
    //don't make the pixels too large or the screen size will be larger than
    //your display size
    pixel_size = 1;
    
    /*Window information*/
    win_height = grid_height*pixel_size;
    win_width = grid_width*pixel_size;
    
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    /*initialize variables, allocate memory, create buffers, etc. */
    //create window of size (win_width x win_height
    glutInitWindowSize(win_width,win_height);
    //windown title is "glut demo"
    glutCreateWindow("glut demo");
    
    /*defined glut callback functions*/
    glutDisplayFunc(display); //rendering calls here
    glutMouseFunc(mouse);     //mouse button events
    glutMotionFunc(motion);   //mouse movement events
    glutKeyboardFunc(key);    //Keyboard events
    glutIdleFunc(idle);       //Function called while program is sitting "idle"
    
    //initialize opengl variables
    init();
    //start glut event loop
    glutMainLoop();
    return 0;
}

/*initialize gl stufff*/
void init()
{
    //set clear color (Default background to white)
    glClearColor(0,0,0,0);
    origin.initialization();
    
    cout << "The default display mode is Bezier Curve\n";
    cout << "If you want to change the curve method,\n";
    cout << "please press B on the graph window and follow the instructions:)\n" << endl;

    //checks for OpenGL errors
    check();
}

//called repeatedly when glut isn't doing anything else
void idle()
{
    //redraw the scene over and over again
    glutPostRedisplay();
}

//this is where we render the screen
void display()
{
    //clears the screen
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    
    if (origin.polys.size() != 0) {
        draw_lines(origin, false);

        for (int i = 0; i < origin.polys.size(); i++) {
            for (int j = 0; j < origin.polys[i].vertices.size(); j++) {
                glPointSize(5);
                draw_pix(origin.polys[i].vertices[j].x , origin.polys[i].vertices[j].y);
            }
            if (isBezier) {
                if (origin.polys[i].vertices.size() > 1) {
                    origin.polys[i].bezPoints.clear();
                    origin.polys[i].bezier();
                }
                if (origin.polys[i].vertices.size() > 1)
                     draw_lines(origin, true);
            } else {
                origin.polys[i].updateCurve();
                if (origin.polys[i].vertices.size() > 1)
                    draw_lines(origin, true);
            }
        }
    }
    
    //blits the current opengl framebuffer on the screen
    glutSwapBuffers();
    //checks for opengl errors
    check();
}


//Draws a single "pixel" given the current grid size
//don't change anything in this for project 1
void draw_pix(int x, int y){
    glBegin(GL_POINTS);
    glColor3f(.2,.2,1.0);
    glVertex3f(x+.5,y+.5,0);
    glEnd();
//    glBegin(GL_POINTS);
//    glColor3f(1.0,0,0);
//    glVertex3f(x,y,0);
//    glEnd();
}

void key(unsigned char ch, int x, int y)
{
    Polygon poly;
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
             for (int i = 0; i < origin.polys.size(); i++) {
                 origin.polys[i].curve = cur;
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
             if (aid > origin.polys.size())
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
             for (int i = 0; i < origin.polys.size(); i++) {
                 origin.polys[i].vertices.clear();
             }
             origin.polys.clear();
             
             temp.x = -300;
             temp.y = -300;
             //vector <Vertex> v;
             //v.push_back(temp);
             
             poly.vertices.push_back(temp);
             origin.polys.push_back(poly);
             //origin.printInfo(1);
             break;
          case 'q' :
          cout << "Thanks for your using :)" <<  endl;
          if (origin.polys.size() == 1 && origin.polys[0].vertices[0].x == -300) {
           fout << "No curve in this graph" << endl;
          }
          else {
            fout << "the number of the curve is: " << origin.polys.size() << endl << endl;
            for (int i = 0; i < origin.polys.size(); i++) {
             fout << "the id of the curve is: " << i+1 << endl << endl;
             fout << "the vertex of the curve is: " << endl;
             for (int j = 0; j < origin.polys[i].vertices.size(); j++) {
               fout << "(" << origin.polys[i].vertices[j].x << "," << origin.polys[i].vertices[j].y << ")" << " --> ";
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
            int l = (int)origin.polys[0].vertices.size();
            if (origin.polys[0].vertices[0].x != -300) {
            if (!((newx >= origin.polys[0].vertices[l-1].x-5 && newx <= origin.polys[0].vertices[l-1].x+5) && (newy >= origin.polys[0].vertices[l-1].y-5 && newy <= origin.polys[0].vertices[l-1].y+5))) {
                origin.polys[0].vertices.push_back(vertex);
                }
            }
                else {
                    origin.polys[0].vertices[0].x = newx;
                    origin.polys[0].vertices[0].y = newy;
                }
        }
        else if (id > 1) {
            if (id > origin.polys.size()) {
                
                Vertex v;
                Polygon poly;
                v.x = newx;
                v.y = newy;
                poly.vertices.push_back(v);
                origin.polys.push_back(poly);
            }
            else {
                Vertex v;
                v.x = newx;
                v.y = newy;
                int l = (int)origin.polys[id-1].vertices.size();
                if (!((newx >= origin.polys[id-1].vertices[l-1].x-5 && newx <= origin.polys[id-1].vertices[l-1].x+5) && (newy >= origin.polys[id-1].vertices[l-1].y-5 && newy <= origin.polys[id-1].vertices[l-1].y+5))) {
                    origin.polys[id-1].vertices.push_back(v);
                }
            }
        }
    }

    if (mousedelete) {
        for (int j = 0; j < origin.polys.size(); j++) {
            for (int i = 0; i < origin.polys[j].vertices.size(); i++) {
                if ((newx >= origin.polys[j].vertices[i].x-5 && newx <= origin.polys[j].vertices[i].x+5) && (newy >= origin.polys[j].vertices[i].y-5 && newy <= origin.polys[j].vertices[i].y+5)) {
                    if (origin.polys[j].vertices.size() != 1)
                        origin.polys[j].vertices.erase(origin.polys[j].vertices.begin() + i);
                        else cout << "only one vertex left, please use clean to delete it" << endl;
                }
            }
        }
    }

    if (mouseadd && aid > 0) {
        bool exist = false;
        for (int i = 0; i < origin.polys[aid-1].vertices.size(); i++) {
            if ((newx >= origin.polys[aid-1].vertices[i].x-5 && newx <= origin.polys[aid-1].vertices[i].x+5) && (newy >= origin.polys[aid-1].vertices[i].y-5 && newy <= origin.polys[aid-1].vertices[i].y+5))
                exist = true;
        }

        if (exist) {
            //cout << "Sorry the point is already existed :(" << endl;
        }
        else {
            Vertex vertex;
            vertex.x = newx;
            vertex.y = newy;
            origin.polys[aid-1].vertices.push_back(vertex);
        }
    }

    if (mousechange && pindex >= 0) {
        origin.polys[pid].vertices[pindex].x = newx;
        origin.polys[pid].vertices[pindex].y = newy;
        //pindex = -1;
    }

    if (mousechange && pindex < 0) {
        for (int j = 0; j < origin.polys.size(); j++) {
            for (int i = 0; i < origin.polys[j].vertices.size(); i++) {
                if ((newx >= origin.polys[j].vertices[i].x-5 && newx <= origin.polys[j].vertices[i].x+5) && (newy >= origin.polys[j].vertices[i].y-5 && newy <= origin.polys[j].vertices[i].y+5)) {
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

