/*
 *
 *  stl2ngc is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  stl2ngc is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with stl2ngc.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef _WIN32
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#else
#include <ctype.h>	
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
//#include <getopt.h>
#include <string.h>	

#endif






//#include <locale>
#include <string>
#include <vector>
#include <iomanip>

//#include <dir.h>

#include <opencamlib/stlsurf.hpp>
#include <opencamlib/stlreader.hpp>
#include <opencamlib/cylcutter.hpp>
#include <opencamlib/adaptivepathdropcutter.hpp>


#include <omp.h>
#include <iostream>
#include <filesystem>
#ifdef _MSC_VER
#include <windows.h>
#include <dos.h>
#endif

using namespace std;
using namespace ocl;



wstring wstr(const char* str) { wstring wstr(str, str + strlen(str)); return wstr; }

class APDC : public AdaptivePathDropCutter {
public:
    APDC() : AdaptivePathDropCutter() {}
    virtual ~APDC() {}
    vector<CLPoint> getPoints() {
        return clpoints;
    }
};
#ifdef _MSC_VER
void SetColor(int ForgC)
{
    WORD wColor;

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    //We use csbi for the wAttributes word.
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi))
    {
        //Mask out all but the background attribute, and add in the forgournd color
        wColor = (csbi.wAttributes & 0xF0) + (ForgC & 0x0F);
        SetConsoleTextAttribute(hStdOut, wColor);
    }
    return;
}
#else
void SetColor(int ForgC)
{
    // dummy function 
}

#endif

Path zigzag_x(double minx, double dx, double maxx,
    double miny, double dy, double maxy, double z) {
    Path p;

    int rev = 0;
    for (double i = miny; i < maxy; i += dy) {
        if (rev == 0) {
            p.append(Line(Point(minx, i, z), Point(maxx, i, z)));
            rev = 1;
        }
        else {
            p.append(Line(Point(maxx, i, z), Point(minx, i, z)));
            rev = 0;
        }
    }

    return p;
}

Path zigzag_y(double minx, double dx, double maxx,
    double miny, double dy, double maxy, double z) {
    Path p;

    int rev = 0;
    for (double i = minx; i < maxx; i += dx) {
        if (rev == 0) {
            p.append(Line(Point(i, miny, z), Point(i, maxy, z)));
            rev = 1;
        }
        else {
            p.append(Line(Point(i, maxy, z), Point(i, miny, z)));
            rev = 0;
        }
    }

    return p;
}


void help()
{
    printf("\n\n");
    printf("And the magic opts are \n");
    printf("-h or --help    This Message \n");
    printf("-s or --zsafe   Z Safe position (default 5.0 mm) \n");
    printf("-d or --zstep   Z Step down (default 3.0 mm) \n");
    printf("-v or --overlap Tool Overlap (default 0.25)  \n");
    printf("-c or --tooldia Cutter dia  (default 2.0 mm) \n");
    printf("-f or --feed    FeedRate (default  200 mm)  \n");
    printf("-r or --spindle Spildle Speed  (default 500) \n");
    printf("-t or --threads Number of threads (default 4 max ) \n");
    printf("-l or --toollen Tool Length (default infile name .ngc ) \n");
    printf("-o or --outfile name of gcode file (default filename - .stl + .ngc ) \n");
    printf("-a or --resize Makes the gcode object size bigger or smaller (default 1.0 ) \n");
    printf("-x or --xplain   offsets gcode in the X plane   (default 0.0 ) \n");
    printf("-y or --yplain   offsets gcode in the Y plane   (default 0.0 ) \n");
    printf("-z or --zplain   offsets gcode in the Z plane   (default 0.0 ) \n");
    printf("Example stl2ngc.exe -s 15.00 -v 0.25 -o example_ruff.ngc -x 0.0 -y 0.0 -z 0.0 -f 500 -r 200 -a 0.75 -d 2.7 -c 6.0 example.stl \n");
    printf("Example stl2ngc.exe --zsafe 15.00 --overlap 0.25 --xplain 0.0 --yplain 0.0 --zplain 0.0 --outfile example_fine.ngc -resize 0.75 --feed 200 --spindle 500  --zstep 50  --toollen 6.0 --tooldia 2.0 --threads 2 example.stl \n");
    printf("\n\n");

    exit(EXIT_FAILURE);
}

static struct option long_options[] =
{
    {"spindle", required_argument, NULL, 'r'},
    {"feed", required_argument, NULL, 'f'},
    {"tooldia", required_argument, NULL, 'c'},
    {"overlap", required_argument, NULL, 'v'},
    {"zstep", required_argument, NULL, 'd'},
    {"zsafe", required_argument, NULL, 's'},
    {"help", required_argument, NULL, 'h'},
    {"threads", required_argument, NULL, 't'},
    {"toollen", required_argument, NULL, 'l'},
    {"outfile", required_argument, NULL, 'o'},
    {"resize", required_argument, NULL, 'a'},
    {"xplain", required_argument, NULL, 'x'},
    {"yplain", required_argument, NULL, 'y'},
    {"zplain", required_argument, NULL, 'z'},
    {NULL, 0, NULL, 0}
};


int main(int argc, char* argv[]) {
    double zsafe = 5; //safe Z
    double zstep = 3; // step down
    int opt;
    double d = 2.0; // tool diamitor
    double tl = 6.0; //tool length
    double d_overlap = 1 - 0.75; // overlap %
    double xplain = 0.0;
    double yplain = 0.0;
    double zplain = 0.0;
    double corner = 0; // d
    //int option_index = 0;
    FILE* fp;
    bool nameset = true;
    string filename2;
#ifdef _MSC_VER 	
    errno_t err;
#endif
    double d_org = 2.0;
    int feedRate = 200;
    int spindleSpeed = 500;
    int g_threads = 4;
    double resize = 1;



    while ((opt = getopt_long(argc, argv, "s:d:v:c:f:r:h:l:t:y:", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            zsafe = atof(optarg);
            printf("Z safe has bin set to %.3f \n", zsafe);
            break;
        case 'd':
            zstep = atof(optarg);
            printf("Z step has bin set to %.3f \n", zstep);
            break;
        case 'v':
            d_overlap = atof(optarg);
            printf("Over lap has bin set to %.3f \n", d_overlap);
            break;
        case 'c':
            d = atof(optarg);
            d_org = atof(optarg);
            printf("Tool Dia. has bin set to %.3f \n", d);
            break;

        case 'f':
            feedRate = atoi(optarg);
            printf("Feed Rate has bin set to %i \n", feedRate);
            break;

        case 'r':
            spindleSpeed = atoi(optarg);
            printf("Spindle Speed has bin set to %i \n", spindleSpeed);
            break;
        case 'h':
            help();

            break;
        case 'l':
            tl = atof(optarg);
            printf("Tool Length has bin set to %.3f \n", tl);
            break;
        case 'o':
            nameset = false;
            filename2 = optarg;
            printf("Out file has bin set to %s \n", filename2.c_str());
            break;
        case 'a':

            resize = atof(optarg);
            printf("Resize has bin set to %.3f \n", resize);
            break;
        case 'x':

            xplain = atof(optarg);
            printf("Xplain has bin set to %.3f \n", xplain);
            break;
        case 'y':

            yplain = atof(optarg);
            printf("Yplain has bin set to %.3f \n", yplain);
            break;
        case 'z':

            zplain = atof(optarg);
            printf("Zplain has bin set to %.3f \n", zplain);
            break;
        case 't':
            g_threads = atoi(optarg);
            if (g_threads > 4)
            {
                g_threads = 4;

            }

            printf("Threads has bin set to %i \n", g_threads);
            break;

        default: /* '?' */
            help();


        }
    }
    if (resize >= 1)
        d = (d / resize);
    else
        d = ((1 - resize) * d) + d;

    if (argc < 2)
    {
        SetColor(4);
        printf("__No input !! __ \n");
        SetColor(7);
        help();


    }
#ifdef _MSC_VER 
    err = fopen_s(&fp, argv[argc - 1], "r");
    if (err == 0)
    {
        fclose(fp);
    }
    else
    {
        SetColor(4);
        printf("__Bad File Name__ \n");
        SetColor(7);
        help();
    }
#else
	fp = fopen(argv[argc - 1], "r");
    if (fp)
    {
        fclose(fp);
    }
    else {
        printf("Bad File Name \n");
        help();
    }

#endif


    string  filename = argv[argc - 1];
    if (filename.find(".stl") == string::npos) {
        SetColor(4);
        printf("__.stl files only__ \n");
        SetColor(7);
        help();
    }
    filename.erase(filename.find(".stl"));
    filename.append(".ngc");

#ifdef _MSC_VER 
    if (nameset)
        err = fopen_s(&fp, filename.c_str(), "w+");
    else
        err = fopen_s(&fp, filename2.c_str(), "w+");
    if (err != 0)
    {
        SetColor(4);
        printf("Something wrong with output file");
        SetColor(7);
        help();

    }
#else
    if (nameset)
        fp = fopen(filename.c_str(), "w+");
    else
        fp = fopen(filename2.c_str(), "w+");

    if (!fp)
    {
        printf("Something wrong with output file");
        help();

    }


#endif





    //printf("zsafe=%f zstep=%f; d_overlap=%f cutter dia = %f filename = %s \n Tool Length = %f threads = %i Feed Rate = %i Spindle Speed = %i \n", zsafe, zstep, d_overlap, d, filename.c_str(), tl, g_threads,feedRate, spindleSpeed);


    fprintf(fp, "(This Project by Richard Rehll )\n");
    fprintf(fp, "(space saver)                   ");
    fprintf(fp, "(LINUX CNC, GRBL GCode Generator  )\n");
    fprintf(fp, "(Resize ratio 1:%.2f  )\n", resize);
    fprintf(fp, "(Tool size: %.2f mm )\n", d_org);
    if (nameset)
        fprintf(fp, "(File name: %s )\n", filename.c_str());
    else
        fprintf(fp, "(File name: %s )\n", filename2.c_str());
    fprintf(fp, "\n\n");

    printf("\n\nstl2ngc  Original program by Jakob Flierl \n");
    printf("This version by Richard Rehll \n");
    printf("This program comes with ABSOLUTELY NO WARRANTY \n");
    printf("This is free software, and you are welcome to redistribute it \n");
    printf("under certain conditions. \n\n");






    STLSurf s;

    STLReader r(wstr(argv[argc - 1]), s);
    // s.rotate(0,0,0);
#ifdef _DEBUG   
    cerr << s << endl;
    cerr << s.bb << endl;

    double zdim = s.bb.maxpt.z - s.bb.minpt.z;
    cerr << "zdim " << zdim << endl;

    double zstart = s.bb.maxpt.z - zstep;
    cerr << "zstart " << zstart << endl;

    CylCutter* c = new CylCutter(d, tl);
    cerr << "Cutter " << *c << endl;
#else

    double zstart = s.bb.maxpt.z - zstep;
    CylCutter* c = new CylCutter(d, tl);
#endif   

    APDC apdc;
    apdc.setSTL(s);
    apdc.setCutter(c);
    apdc.setSampling(d * d_overlap);
    apdc.setMinSampling(d * d_overlap / 100);

    double minx, miny, maxx, maxy, z;
    minx = s.bb.minpt.x - corner;
    miny = s.bb.minpt.y - corner;
    maxx = s.bb.maxpt.x + corner;
    maxy = s.bb.maxpt.y + corner;
    z = s.bb.minpt.z - zsafe;

    double dx = d * d_overlap, dy = d * d_overlap;
    double width, height, lines, passes, timeperpass;
    width = maxy - miny;
    height = maxx - minx;
    lines = d - (d * d_overlap);
    passes = height / lines;
    timeperpass = width / feedRate;


    Path p = zigzag_x(minx, dx, maxx, miny, dy, maxy, z);
    apdc.setPath(&p);
    apdc.setZ(z);


    printf("Calculating... \n");

    apdc.setThreads(g_threads);
    apdc.run();

    printf("Done \n ");


    fprintf(fp, "G21 F%i \n", feedRate);
    fprintf(fp, "G64 P0.001 \n");
    fprintf(fp, "M03 S%i \n", spindleSpeed);
    fprintf(fp, "G00 X%f Y%f Z%f \n", s.bb.minpt.x, s.bb.minpt.y, zsafe);
    double zcurr = zstart;

    vector<CLPoint> pts = apdc.getPoints();
    int nub = 0;
    bool fst = true;
    while (zcurr > s.bb.minpt.z - zstep) {


        printf("Z Current %.2f \n", zcurr);
        nub++;



        BOOST_FOREACH(Point cp, pts) {
            double z = -fmin(-cp.z, -zcurr) - s.bb.maxpt.z;
            double x, y;
            x = (cp.x * resize) + xplain;
            y = (cp.y * resize) + yplain;
            z = (z * resize) + zplain;

            if (fst) {
                // fprintf(fp, "G00 X%f Y%f Z%f \n", cp.x, cp.y, zsafe);
                fprintf(fp, "G00 X%f Y%f Z%f \n", cp.x, cp.y, 0.000);

                fst = false;
            }


            fprintf(fp, "G01 X%f Y%f Z%f \n", x, y, z);
            // fprintf(fp, "G01 X%f Y%f Z%f \n", cp.x, cp.y, z);

        }



        zcurr -= zstep;
        reverse(pts.begin(), pts.end());
    }



    fprintf(fp, "G00  Z%f \n", zsafe);
    fprintf(fp, "M05 \n");
    fprintf(fp, "G00 X%f Z%f \n", s.bb.minpt.x, s.bb.minpt.y);
    fprintf(fp, "M2 \n");
    fprintf(fp, "%% \n");
    double temp;

    int days = floor(timeperpass * passes * nub * resize) / (24 * 60);
    temp = (timeperpass * passes * nub) - (days * 24 * 60);

    int hours1 = floor(temp / 60);
    temp = temp - ( hours1 * 60 );
    int minutes1 = round(temp);

    printf("Job Time: Days:H:M %0i:%0i:%0i  \n", days, hours1, minutes1);
    fseek(fp, 34, SEEK_SET);
    fprintf(fp, "(Job Time: Days:H:M %0i:%0i:%0i  )\n", days, hours1, minutes1);

    fclose(fp);
    return 0;
}


