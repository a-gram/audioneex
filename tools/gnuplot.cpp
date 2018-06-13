#ifdef WIN32
#include <io.h>        // for _pipe(), _dup()
#else
#include <unistd.h>
#endif

#include <fcntl.h>     // for O_TEXT/BINARY
//#include <process.h>   // for _exec()
#include <assert.h>
#include <time.h>

#include <cmath>
#include <cerrno>

#include "gnuplot.h"

//
// initialize static data
//
int                      Gnuplot::mPlotID = 0;
int                      Gnuplot::tmpfile_num = 0;
FILE*                    Gnuplot::gnucmd = NULL;
std::string              Gnuplot::mTempFilesDir;
Gnuplot::ePlotDevice     Gnuplot::mPlotDevice = Gnuplot::WindowPlotDevice;
std::list<std::string>   Gnuplot::mPlotList;
std::vector<std::string> Gnuplot::tmpfile_list;


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
std::string Gnuplot::m_sGNUPlotFileName = "gnuplot.exe";
std::string Gnuplot::m_sGNUPlotPath = "C:\\program files\\gnuplot\\bin";
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
std::string Gnuplot::m_sGNUPlotFileName = "gnuplot";
std::string Gnuplot::m_sGNUPlotPath = "/usr/local/bin/";
#endif

//------------------------------------------------------------------------------
//
// constructor: set a style during construction
//
Gnuplot::Gnuplot(const std::string &style) :
    two_dim(false) ,
    nplots(0),
    mPersist(true),
    mPlotMode(Normal)
{
    if(gnucmd==NULL)
       throw std::runtime_error("GnuPlot not initialized. Please call Gnuplot::Init().");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    setWindowPlotDevice("windows", mPlotID++);
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    setWindowPlotDevice("wxt", mPlotID++);
#endif
    set_style(style);
}

//------------------------------------------------------------------------------
//
// constructor: open a new session, plot a signal (x)
//
Gnuplot::Gnuplot(const std::vector<double> &x,
                 const std::string &title,
                 const std::string &style,
                 const std::string &labelx,
                 const std::string &labely) :
//    valid(false) ,
    two_dim(false) ,
    nplots(0)
{
    if(gnucmd==NULL)
       throw std::runtime_error("GnuPlot not initialized.");

    set_style(style);
    set_xlabel(labelx);
    set_ylabel(labely);

    plot_x(x,title);
}


//------------------------------------------------------------------------------
//
// constructor: open a new session, plot a signal (x,y)
//
Gnuplot::Gnuplot(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::string &title,
                 const std::string &style,
                 const std::string &labelx,
                 const std::string &labely) :
//    valid(false) ,
    two_dim(false) ,
    nplots(0),
    mPersist(true)
{
    if(gnucmd==NULL)
       throw std::runtime_error("GnuPlot not initialized.");

    set_style(style);
    set_xlabel(labelx);
    set_ylabel(labely);

    plot_xy(x,y,title);
}


//------------------------------------------------------------------------------
//
// constructor: open a new session, plot a signal (x,y,z)
//
Gnuplot::Gnuplot(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::vector<double> &z,
                 const std::string &title,
                 const std::string &style,
                 const std::string &labelx,
                 const std::string &labely,
                 const std::string &labelz) :
//    valid(false) ,
    two_dim(false) ,
    nplots(0),
    mPersist(true)
{
    if(gnucmd==NULL)
       throw std::runtime_error("GnuPlot not initialized.");

    set_style(style);
    set_xlabel(labelx);
    set_ylabel(labely);
    set_zlabel(labelz);

    plot_xyz(x,y,z,title);
}

//------------------------------------------------------------------------------

/* static */
void Gnuplot::Init()
{
    // char * getenv ( const char * name );  get value of environment variable
    // Retrieves a C string containing the value of the environment variable
    // whose name is specified as argument.  If the requested variable is not
    // part of the environment list, the function returns a NULL pointer.
#if ( defined(unix) || defined(__unix) || defined(__unix__) ) && !defined(__APPLE__)
    if (getenv("DISPLAY") == NULL)
    {
        //valid = false;
        throw GnuplotException("Can't find DISPLAY variable");
    }
#endif

    // check that gnuplot is available and set its path if so
    if (!Gnuplot::get_program_path())
    {
        //valid = false;
        throw GnuplotException("Can't find gnuplot");
    }

    //
    // open pipe
    //
    std::string cmd = Gnuplot::m_sGNUPlotPath + "/" +
                      Gnuplot::m_sGNUPlotFileName;

    // execute gnuplot in persist mode, if flag is set
    //if(mPersist)
       //tmp.append(" -persist");

    // FILE *popen(const char *command, const char *mode);
    // The popen() function shall execute the command specified by the string
    // command, create a pipe between the calling program and the executed
    // command, and return a pointer to a stream that can be used to either read
    // from or write to the pipe.

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    gnucmd = _popen(cmd.c_str(),"w");
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    gnucmd = popen(cmd.c_str(),"w");
#endif

    std::string perr = strerror(errno);

    // popen() shall return a pointer to an open stream that can be used to read
    // or write to the pipe.  Otherwise, it shall return a null pointer and may
    // set errno to indicate the error.
    if (!gnucmd)
    {
        //valid = false;
        throw GnuplotException("Couldn't open connection to gnuplot. "+perr);
    }

}

//------------------------------------------------------------------------------

/* static */
void Gnuplot::Terminate()
{
    if(gnucmd != NULL){
       // Close the pipe to gnuplot.
       // A stream opened by popen() should be closed by pclose()
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
       if (_pclose(gnucmd) == -1)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
       if (pclose(gnucmd) == -1)
#endif
           std::cerr << "Problem closing communication to gnuplot" << std::endl;
    }
    // remove temp files
    remove_tmpfiles();
}

//------------------------------------------------------------------------------
//
// Find out if a command lives in m_sGNUPlotPath or in PATH
//
bool Gnuplot::get_program_path()
{
    //
    // first look in m_sGNUPlotPath for Gnuplot
    //
    std::string tmp = Gnuplot::m_sGNUPlotPath + "/" +
                      Gnuplot::m_sGNUPlotFileName;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if ( Gnuplot::file_exists(tmp,0) ) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if ( Gnuplot::file_exists(tmp,1) ) // check existence and execution permission
#endif
    {
        return true;
    }


    //
    // second look in PATH for Gnuplot
    //
    char *path;
    // Retrieves a C string containing the value of environment variable PATH
    path = getenv("PATH");


    if (path == NULL)
    {
        throw GnuplotException("Path is not set");
        return false;
    }
    else
    {
        std::list<std::string> ls;

        //split path (one long string) into list ls of strings
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
        stringtok(ls,path,";");
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
        stringtok(ls,path,":");
#endif

        // scan list for Gnuplot program files
        for (std::list<std::string>::const_iterator i = ls.begin();
                i != ls.end(); ++i)
        {
            tmp = (*i) + "/" + Gnuplot::m_sGNUPlotFileName;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
            if ( Gnuplot::file_exists(tmp,0) ) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
            if ( Gnuplot::file_exists(tmp,1) ) // check existence and execution permission
#endif
            {
                Gnuplot::m_sGNUPlotPath = *i; // set m_sGNUPlotPath
                return true;
            }
        }

        tmp = "Can't find gnuplot neither in PATH nor in \"" +
              Gnuplot::m_sGNUPlotPath + "\"";

        Gnuplot::m_sGNUPlotPath = "";
        return false;
    }
}



//------------------------------------------------------------------------------
//
// check if file exists
//
bool Gnuplot::file_exists(const std::string &filename, int mode)
{
    if ( mode < 0 || mode > 7)
    {
        throw std::runtime_error("In function \"Gnuplot::file_exists\": mode"
                                 "has to be an integer between 0 and 7");
        return false;
    }

    // int _access(const char *path, int mode);
    //  returns 0 if the file has the given mode,
    //  it returns -1 if the named file does not exist or is not accessible in
    //  the given mode
    // mode = 0 (F_OK) (default): checks file for existence only
    // mode = 1 (X_OK): execution permission
    // mode = 2 (W_OK): write permission
    // mode = 4 (R_OK): read permission
    // mode = 6       : read and write permission
    // mode = 7       : read, write and execution permission
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if (_access(filename.c_str(), mode) == 0)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if (access(filename.c_str(), mode) == 0)
#endif
    {
        return true;
    }
    else
    {
        return false;
    }

}

//------------------------------------------------------------------------------

bool Gnuplot::file_available(const std::string &filename)
{
    std::ostringstream except;
    if( Gnuplot::file_exists(filename,0) ) // check existence
    {
        if( !(Gnuplot::file_exists(filename,4)) ){// check read permission
            except << "No read permission for File \"" << filename << "\"";
            throw GnuplotException( except.str() );
            return false;
        }
    }
    else{
        except << "File \"" << filename << "\" does not exist";
        throw GnuplotException( except.str() );
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
//
// Opens a temporary file
//
std::string Gnuplot::create_tmpfile(std::ofstream &tmp, bool binary)
{
    // create a unique id for the temporary file

    std::ostringstream ifile;

    srand(time(NULL));
    int uid = rand();

    do{
        uid++;
        ifile.str("");
        ifile << mTempFilesDir << "_gnuplot_" << mPlotID << "_" << uid << ".gptmp";
    // check for collisions
    }while(file_exists(ifile.str()));

    // Try to open the file (binary or text mode according to the set flag)
    binary ? tmp.open(ifile.str().c_str(), std::ios::out|std::ios::binary) :
             tmp.open(ifile.str().c_str());

    if (tmp.bad())
    {
        std::ostringstream except;
        except << "Cannot create temporary file \"" << ifile.str() << "\"";
        throw GnuplotException(except.str());
        return "";
    }

    //
    // Save the temporary filename
    //
    tmpfile_list.push_back(ifile.str());

    Gnuplot::tmpfile_num++;

    return ifile.str();
}

//------------------------------------------------------------------------------

void Gnuplot::remove_tmpfiles()
{
    if (tmpfile_list.size() > 0)
    {
        for (unsigned int i = 0; i < tmpfile_list.size(); i++)
            remove( tmpfile_list[i].c_str() );

        Gnuplot::tmpfile_num -= tmpfile_list.size();

        tmpfile_list.clear();
    }
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::plot_map(const std::vector<std::vector<float> > &amap,
                           const std::vector<float> &XLabels,
                           const std::vector<float> &YLabels,
                           const std::string &title,
                           const bool AutoRange)
{
    if (amap.size() == 0){
        return *this;
    }

    std::ofstream tmpfile;
    std::string filename = create_tmpfile(tmpfile, true);

    if(filename == "")
       return *this;

    // get the number of rows and columns of the map's matrix
    size_t mrows = amap[0].size();
    size_t mcols = amap.size();

    // create the map's matrix
    float *mat = new float[mrows*mcols];

    // initialize min/max values in the map
    float vmin = amap[0][0];
    float vmax = vmin;

    // check the axes lables vectors if provided, create them if not

    bool createXLabels = false;
    bool createYLabels = false;

    std::vector<float>& XLabels_ = const_cast<std::vector<float>&>(XLabels);
    std::vector<float>& YLabels_ = const_cast<std::vector<float>&>(YLabels);

    if(!XLabels_.empty())
       assert(XLabels_.size() == mcols);
    else
    {
       XLabels_.resize(mcols);
       createXLabels=true;
    }

    if(!YLabels_.empty())
       assert(YLabels_.size() == mrows);
    else
    {
       YLabels_.resize(mrows);
       createYLabels=true;
    }

    // fill in the matrix with the maps' values

    std::vector<std::vector<float> >::const_iterator it;
    float val;
    size_t col,row;

    for(row=0; row<mrows; row++)
    {
        for(col=0, it=amap.begin(); it!=amap.end(); ++it, col++)
        {
            val = (*it)[row];
            // find min/max values in the map
            vmin = std::min<float>(val,vmin);
            vmax = std::max<float>(val,vmax);

            mat[row*mcols+col]=val;

            // create a x label if it's not provided
            if(createXLabels)
               XLabels_[col] = col;
        }

        // create a y label if it's not provided
        if(createYLabels)
           YLabels_[row] = row;
    }


    // write the binary matrix to the temp file
    write_matrix(tmpfile, mat, YLabels_, XLabels_);

    tmpfile.flush();
    tmpfile.close();

    delete[] mat;

    // command to be sent to gnuplot

    std::ostringstream cmdstr;

    cmdstr << "set view map" << std::endl
           << "unset key"  << std::endl;

    if(!title.empty())
       cmdstr << "set title \"" << title << "\"" << std::endl;

    // in multiplot mode don't set margins (else the plot will be forced to take all available space)
//    if(mPlotMode!=MultiplotMode)
//    {
//       cmdstr<< "set tmargin at screen 0.90"  << std::endl
//             << "set bmargin at screen 0.10"  << std::endl
//             << "set rmargin at screen 0.90"  << std::endl
//             << "set lmargin at screen 0.08"  << std::endl;
//    }

    cmdstr << "set palette rgb 7,5,15;"  << std::endl;

    // if auto range specified, use computed min/max values
    if(AutoRange)
       cmdstr << "set cbrange ["<<vmin<<":"<<vmax<<"]" << std::endl;

    cmdstr << "set xrange [" << XLabels[0] << ":" << XLabels[mcols-1] << "]" << std::endl
           << "set yrange [" << YLabels[0] << ":" << YLabels[mrows-1] << "]" << std::endl;

    cmdstr << "set xtics out" << std::endl;
    cmdstr << "set ytics out" << std::endl;

    cmd(cmdstr.str());

    cmdstr.str("");
    cmdstr << "'" << filename << "' binary matrix with image";


    // For file-type plot devices (read terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("splot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd("splot "+cmdstr.str());
    }

    // release output
    if(mPlotMode!=MultiplotMode && mPlotDevice==WindowPlotDevice)
       cmd("set output");

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::plot_2D_points(const std::vector<std::vector<float> > &points,
                                 bool isMap,
                                 const std::vector<float> &px,
                                 const std::vector<float> &py,
                                 const std::string &style,
                                 const std::string &title)
{
    if(points.size() == 0) return *this;

    std::ofstream tmp;
    std::string name = create_tmpfile(tmp);

    if (name == "")
        return *this;

    if(isMap){

        if(!px.empty()) assert(px.size() == points.size());
        if(!py.empty()) assert(py.size() == points[0].size());

        for (size_t x = 0; x < points.size(); x++)
            for (size_t y = 0; y < points[0].size(); y++)
                if(points[x][y] != 0)
                    tmp << ((px.empty()) ? x : px[x]) << " "
                        << ((py.empty()) ? y : py[y]) << std::endl;
    }
    else
    {
        for(std::vector<std::vector<float> >::const_iterator it=points.begin();
            it!=points.end();
            ++it)
        {
            const std::vector<float> &point = (*it);
            assert(point.size() == 2);
            tmp << point[0] << " " << point[1] << std::endl;
        }
    }

    tmp.flush();
    tmp.close();

    file_available(name);

    std::ostringstream cmdstr;

    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << "'" << name << "' using 1:2:(0)";

    if (title == "")
        cmdstr << " notitle ";
    else
        cmdstr << " title \"" << title << "\" ";

    cmdstr << "with " << style;

    // For file-type plot devices (read terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::plot_2D_boxes(const std::list<std::vector<float> > &boxes,
                                const std::string &color,
                                const std::string &title)
{
    if(boxes.empty())
       return *this;

    std::ostringstream cmdstr;
    int oid=0;

    for(std::list<std::vector<float > >::const_iterator it = boxes.begin();
        it!=boxes.end();
        ++it)
    {
        std::vector<float> r = (*it);
        assert(r.size()==4);

        cmdstr << "set object " << ++oid << " rect "
               << "from " << r[0] << "," << r[1] << " "
               << "to " << r[2] << "," << r[3] << " front "
               << "fs empty border rgb \""<< color <<"\"" << std::endl;

    }

    cmd(cmdstr.str());
    cmdstr.str("");

    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << "NaN";

    if (title == "")
        cmdstr << " notitle ";
    else
        cmdstr << " title \"" << title << "\" ";

    //cmdstr << "with line lw 0 lc rgb \"#FFFFFFFF\"";

    // For file-type plot devices (read terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
        cmd("unset object");
    }

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::plot_2D_text(const std::list<std::pair< std::string, std::vector<float> > > &labels,
                               const std::string &font,
                               const std::string &color,
                               const std::string &title)
{
    if(labels.empty())
       return *this;

    std::ostringstream cmdstr;
    int oid=0;

    for(std::list<std::pair< std::string, std::vector<float> > >::const_iterator it = labels.begin();
        it!=labels.end();
        ++it)
    {
        std::pair< std::string, std::vector<float> > p = (*it);

        assert(p.second.size()>=2);

        // set label 1 "text" font "font_name,font_size" at x,y right textcolor rgbcolor "#RRGGBB"

        cmdstr << "set label " << ++oid << " \"" << p.first << "\" "
               << "at " << p.second[0] << "," << p.second[1] << " "
               << "font \""<< font <<"\" textcolor rgb \"" << color
               <<"\" front"<< std::endl;

    }

    cmd(cmdstr.str());
    cmdstr.str("");

    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << "NaN";

    if (title == "")
        cmdstr << " notitle ";
    else
        cmdstr << " title \"" << title << "\" ";

    //cmdstr << "with line lw 0 lc rgb \"#FFFFFFFF\"";

    // For file-type plot devices (read terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::plot_histogram(const std::vector<std::vector<float> > &groups,
                                 const std::vector<std::string> &labels,
                                 const std::string &style,
                                 const std::string &title)
{
    assert(groups.size() > 0);

    // all groups and labels must be the same size
    size_t gsize = groups[0].size();

    for(size_t i=0; i<groups.size(); ++i)
        assert(groups[i].size() == gsize);

    if(!labels.empty())
        assert(labels.size() == gsize);

    std::ofstream tmp;
    std::string name = create_tmpfile(tmp);

    if (name == "")
        return *this;

    for(size_t i = 0; i < gsize; i++){
       if(labels.empty()) tmp << i; else tmp << labels[i];
       tmp << " ";
       for(size_t g = 0; g < groups.size(); g++){
           tmp << groups[g][i];
           if(g==groups.size()-1) tmp << std::endl; else tmp << " ";
       }
    }

    tmp.flush();
    tmp.close();

    file_available(name);

    std::ostringstream cmdstr;

    cmdstr << "unset key;" << std::endl;

    if(!title.empty())
       cmdstr << "set title \"" << title << "\"" << std::endl;

    cmdstr << "set style data histogram" << std::endl;
    cmdstr << "set style histogram clustered gap 0" << std::endl;
    cmdstr << "set style " << style << std::endl;
    cmdstr << "set style line 1 lc rgb \"#888888\"" << std::endl;
    cmdstr << "set xtics nomirror rotate by -45 scale 0 font \",8\"" << std::endl;

    cmd(cmdstr.str());
    cmdstr.str("");

    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << "for [col=2:" << (groups.size()+1) << "] '" << name
           << "' using col:xticlabels(1) ls 1;";


    // For file-type plot devices (read terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;
}

//------------------------------------------------------------------------------

int Gnuplot::write_matrix(std::ofstream &fout, float *m,
                          std::vector<float> &row_title,
                          std::vector<float> &column_title)
{
    float *clabels;

    size_t ncols = column_title.size();
    size_t nrows = row_title.size();

    if(ncols<=0 || nrows<=0){
       std::cerr << "write_matrix() Invalid number of columns/rows" << std::endl;
       return -1;
    }

    clabels = new float[ncols];

    for(size_t i=0; i<ncols; i++)
        clabels[i]=column_title[i];

    // # of columns stored in a float as it's also used to determine
    // the matrix elements size.
    float cols = ncols;

    // write # of columns field to file
    fout.write((char*)&cols, sizeof(float));

    // write the columns labels (x-axis values)
    fout.write((char*)clabels, ncols * sizeof(float));

    // write the matrix along with the rows labels (y-axis values)
    for (size_t offset=0, j=0; j<nrows; j++, offset+=ncols) {
        fout.write((char*)&row_title[j], sizeof(float));
        fout.write((char*)(m+offset), ncols * sizeof(float));
    }

//    for (size_t  j=0; j<nrows; j++) {
//        fout.write((char*)&row_title[j], sizeof(float));
//        fout.write((char*)m[j], ncols * sizeof(float));
//    }

    delete clabels;

    return 0;
}

//------------------------------------------------------------------------------

void Gnuplot::FlushPlots()
{
    if(mPlotDevice==PngPlotDevice ||
       mPlotDevice==PSPlotDevice)
    {
        if(!mPlotList.empty())
        {
           std::string cmds;
           for(std::list<std::string>::iterator it=mPlotList.begin();
               it!=mPlotList.end();
               ++it)
               cmds.append(*it);

           cmd(cmds);

           cmd("set output");
           cmd("unset object");

           mPlotList.clear();
        }
    }
}

//------------------------------------------------------------------------------

//
// define static member function: set Gnuplot path manual
//   for windows: path with slash '/' not backslash '\'
//
bool Gnuplot::setGNUPlotPath(const std::string &path)
{

    std::string tmp = path + "/" + Gnuplot::m_sGNUPlotFileName;


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if ( Gnuplot::file_exists(tmp,0) ) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if ( Gnuplot::file_exists(tmp,1) ) // check existence and execution permission
#endif
    {
        Gnuplot::m_sGNUPlotPath = path;
        return true;
    }
    else
    {
        Gnuplot::m_sGNUPlotPath.clear();
        return false;
    }
}


//------------------------------------------------------------------------------

bool Gnuplot::setTempFilesDir(const std::string &path)
{
    mTempFilesDir = path;

    return true;
}

//------------------------------------------------------------------------------
//
// define static member function: set standart terminal, used by showonscreen
//  defaults: Windows - win, Linux - x11, Mac - aqua
//
void Gnuplot::set_terminal_std(const std::string &type)
{
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if (type.find("x11") != std::string::npos && getenv("DISPLAY") == NULL)
    {
        throw GnuplotException("Can't find DISPLAY variable");
    }
#endif

    //Gnuplot::terminal_std = type;
}


//------------------------------------------------------------------------------

//
// Destructor: needed to delete temporary files
//
Gnuplot::~Gnuplot()
{
    // Flush all plots on hold for drawing, if any
    FlushPlots();

}

//------------------------------------------------------------------------------

//
// Resets a gnuplot session (next plot will erase previous ones)
//
Gnuplot& Gnuplot::reset_plot()
{
//    remove_tmpfiles();

    nplots = 0;

    return *this;
}


//------------------------------------------------------------------------------
//
// resets a gnuplot session and sets all varibles to default
//
Gnuplot& Gnuplot::reset_all()
{
//    remove_tmpfiles();

    nplots = 0;
    cmd("reset");
    cmd("clear");
    pstyle = "points";
    smooth = "";
    //showonscreen();

    return *this;
}


//------------------------------------------------------------------------------
//
// Change the plotting style of a gnuplot session
//
Gnuplot& Gnuplot::set_style(const std::string &stylestr)
{
    if (stylestr.find("lines")          == std::string::npos  &&
        stylestr.find("points")         == std::string::npos  &&
        stylestr.find("linespoints")    == std::string::npos  &&
        stylestr.find("impulses")       == std::string::npos  &&
        stylestr.find("dots")           == std::string::npos  &&
        stylestr.find("steps")          == std::string::npos  &&
        stylestr.find("fsteps")         == std::string::npos  &&
        stylestr.find("histeps")        == std::string::npos  &&
        stylestr.find("boxes")          == std::string::npos  &&  // 1-4 columns of data are required
        stylestr.find("filledcurves")   == std::string::npos  &&
        stylestr.find("histograms")     == std::string::npos  )   //only for one data column
//        stylestr.find("labels")         == std::string::npos  &&  // 3 columns of data are required
//        stylestr.find("xerrorbars")     == std::string::npos  &&  // 3-4 columns of data are required
//        stylestr.find("xerrorlines")    == std::string::npos  &&  // 3-4 columns of data are required
//        stylestr.find("errorbars")      == std::string::npos  &&  // 3-4 columns of data are required
//        stylestr.find("errorlines")     == std::string::npos  &&  // 3-4 columns of data are required
//        stylestr.find("yerrorbars")     == std::string::npos  &&  // 3-4 columns of data are required
//        stylestr.find("yerrorlines")    == std::string::npos  &&  // 3-4 columns of data are required
//        stylestr.find("boxerrorbars")   == std::string::npos  &&  // 3-5 columns of data are required
//        stylestr.find("xyerrorbars")    == std::string::npos  &&  // 4,6,7 columns of data are required
//        stylestr.find("xyerrorlines")   == std::string::npos  &&  // 4,6,7 columns of data are required
//        stylestr.find("boxxyerrorbars") == std::string::npos  &&  // 4,6,7 columns of data are required
//        stylestr.find("financebars")    == std::string::npos  &&  // 5 columns of data are required
//        stylestr.find("candlesticks")   == std::string::npos  &&  // 5 columns of data are required
//        stylestr.find("vectors")        == std::string::npos  &&
//        stylestr.find("image")          == std::string::npos  &&
//        stylestr.find("rgbimage")       == std::string::npos  &&
//        stylestr.find("pm3d")           == std::string::npos  )
    {
        pstyle = std::string("points");
    }
    else
    {
        pstyle = stylestr;
    }

    return *this;
}


//------------------------------------------------------------------------------
//
// smooth: interpolation and approximation of data
//
Gnuplot& Gnuplot::set_smooth(const std::string &stylestr)
{
    if (stylestr.find("unique")    == std::string::npos  &&
        stylestr.find("frequency") == std::string::npos  &&
        stylestr.find("csplines")  == std::string::npos  &&
        stylestr.find("acsplines") == std::string::npos  &&
        stylestr.find("bezier")    == std::string::npos  &&
        stylestr.find("sbezier")   == std::string::npos  )
    {
        smooth = "";
    }
    else
    {
        smooth = stylestr;
    }

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::setQtWidgetPlotDevice(const std::string &filename)
{
    setPngPlotDevice(filename);

    mPlotDevice = QtWidgetDevice;

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::setWindowPlotDevice(const std::string &win_system, int wid)
{
    // if a file device is already set in gnuplot,
    // then flush all pending plots, if any
    FlushPlots();

    std::ostringstream swid;
    swid << wid;

    if(wid<0)
       cmd("set terminal " + win_system);
    else
       cmd("set terminal " + win_system + " " + swid.str());

    cmd("set output");

    mPlotDevice = WindowPlotDevice;

    return *this;
}

//------------------------------------------------------------------------------
//
// saves a gnuplot session to a postscript file
//
Gnuplot& Gnuplot::setPostscriptPlotDevice(const std::string &filename)
{
    // if a ps device is already set in gnuplot,
    // then flush all pending plots, if any
    FlushPlots();

    cmd("set terminal postscript color");

    std::ostringstream cmdstr;
    cmdstr << "set output \"" << filename << ".ps\"";
    cmd(cmdstr.str());

    mPlotDevice = PSPlotDevice;

    return *this;
}

//------------------------------------------------------------------------------
//
// saves a gnuplot session to a png file
//
Gnuplot& Gnuplot::setPngPlotDevice(const std::string &filename, const int width, const int height)
{
    // if a png device is already set in gnuplot,
    // then flush all pending plots, if any
    FlushPlots();

    std::ostringstream cmdstr;
    cmdstr << "set terminal pngcairo size " << width << "," << height << " enhanced font 'Verdana,9'" <<std::endl;
    cmdstr << "set output \'" << filename <<"\'"<<std::endl;

    mPlotImageFile = filename;

    cmd(cmdstr.str());

    mPlotDevice = PngPlotDevice;

    return *this;
}

//------------------------------------------------------------------------------
//
// Switches legend on
//
Gnuplot& Gnuplot::set_legend(const std::string &position)
{
    std::ostringstream cmdstr;
    cmdstr << "set key " << position;

    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// turns on log scaling for the x axis
//
Gnuplot& Gnuplot::set_xlogscale(const double base)
{
    std::ostringstream cmdstr;

    cmdstr << "set logscale x " << base;
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// turns on log scaling for the y axis
//
Gnuplot& Gnuplot::set_ylogscale(const double base)
{
    std::ostringstream cmdstr;

    cmdstr << "set logscale y " << base;
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// turns on log scaling for the z axis
//
Gnuplot& Gnuplot::set_zlogscale(const double base)
{
    std::ostringstream cmdstr;

    cmdstr << "set logscale z " << base;
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// scales the size of the points used in plots
//
Gnuplot& Gnuplot::set_pointsize(const double pointsize)
{
    std::ostringstream cmdstr;
    cmdstr << "set pointsize " << pointsize;
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// set isoline density (grid) for plotting functions as surfaces
//
Gnuplot& Gnuplot::set_samples(const int samples)
{
    std::ostringstream cmdstr;
    cmdstr << "set samples " << samples;
    cmd(cmdstr.str());

    return *this;
}


//------------------------------------------------------------------------------
//
// set isoline density (grid) for plotting functions as surfaces
//
Gnuplot& Gnuplot::set_isosamples(const int isolines)
{
    std::ostringstream cmdstr;
    cmdstr << "set isosamples " << isolines;
    cmd(cmdstr.str());

    return *this;
}


//------------------------------------------------------------------------------
//
// enables contour drawing for surfaces set contour {base | surface | both}
//

Gnuplot& Gnuplot::set_contour(const std::string &position)
{
    if (position.find("base")    == std::string::npos  &&
        position.find("surface") == std::string::npos  &&
        position.find("both")    == std::string::npos  )
    {
        cmd("set contour base");
    }
    else
    {
        cmd("set contour " + position);
    }

    return *this;
}

//------------------------------------------------------------------------------
//
// set labels
//
// set the xlabel
Gnuplot& Gnuplot::set_xlabel(const std::string &label)
{
    std::ostringstream cmdstr;

    cmdstr << "set xlabel \"" << label << "\"";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
// set the ylabel
//
Gnuplot& Gnuplot::set_ylabel(const std::string &label)
{
    std::ostringstream cmdstr;

    cmdstr << "set ylabel \"" << label << "\"";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
// set the zlabel
//
Gnuplot& Gnuplot::set_zlabel(const std::string &label)
{
    std::ostringstream cmdstr;

    cmdstr << "set zlabel \"" << label << "\"";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// set range
//
// set the xrange
Gnuplot& Gnuplot::set_xrange(const double iFrom,
                             const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set xrange[" << iFrom << ":" << iTo << "]";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
// set the yrange
//
Gnuplot& Gnuplot::set_yrange(const double iFrom,
                             const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set yrange[" << iFrom << ":" << iTo << "]";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
// set the zrange
//
Gnuplot& Gnuplot::set_zrange(const double iFrom,
                             const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set zrange[" << iFrom << ":" << iTo << "]";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// set the palette range
//
Gnuplot& Gnuplot::set_cbrange(const double iFrom,
                              const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set cbrange[" << iFrom << ":" << iTo << "]";
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::set_grid_layout(const int rows, const int cols)
{
    std::ostringstream cmdstr;

    cmdstr << "set multiplot layout " << rows << "," << cols;
    cmd(cmdstr.str());

    mPlotMode = MultiplotMode;

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::set_grid_geometry(double x, double y, double w, double h)
{
    std::ostringstream cmdstr;

    cmdstr << "set origin " << x << "," << y << std::endl
           << "set size " << w << "," << h << std::endl;

    cmd(cmdstr.str());

    mPlotMode = MultiplotMode;

    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::unset_grid_layout()
{
    cmd("unset multiplot");
    mPlotMode = Normal;
    return *this;
}

//------------------------------------------------------------------------------

Gnuplot& Gnuplot::set_grid_lines(float xstep, float ystep)
{
    std::ostringstream cmdstr;

    cmdstr << "set xtics nomirror out "  << xstep << std::endl;
    cmdstr << "set ytics nomirror out "  << ystep << std::endl;
    cmdstr << "set grid xtics mxtics ytics mytics" << std::endl;
    cmd(cmdstr.str());
    return *this;
}

//------------------------------------------------------------------------------
//
// Plots a linear equation y=ax+b (where you supply the
// slope a and intercept b)
//
Gnuplot& Gnuplot::plot_slope(const double a,
                             const double b,
                             const std::string &title)
{
    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0  &&  two_dim == true)
        cmdstr << "replot ";
    else
        cmdstr << "plot ";

    cmdstr << a << " * x + " << b << " title \"";

    if (title == "")
        cmdstr << "f(x) = " << a << " * x + " << b;
    else
        cmdstr << title;

    cmdstr << "\" with " << pstyle;

    //
    // Do the actual plot
    //
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// Plot an equation supplied as a std::string y=f(x) (only f(x) expected)
//
Gnuplot& Gnuplot::plot_equation(const std::string &equation,
                                const std::string &title)
{
    std::ostringstream cmdstr;

    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0  &&  two_dim == true)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << equation << " title \"";

    if (title == "")
        cmdstr << "f(x) = " << equation;
    else
        cmdstr << title;

    cmdstr << "\" with " << pstyle;

    // For file-type plot devices (terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;
}

//------------------------------------------------------------------------------
//
// plot an equation supplied as a std::string y=f(x,y)
//
Gnuplot& Gnuplot::plot_equation3d(const std::string &equation,
                                  const std::string &title)
{
    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0  &&  two_dim == false)
        cmdstr << "replot ";
    else
        cmdstr << "splot ";

    cmdstr << equation << " title \"";

    if (title == "")
        cmdstr << "f(x,y) = " << equation;
    else
        cmdstr << title;

    cmdstr << "\" with " << pstyle;

    //
    // Do the actual plot
    //
    cmd(cmdstr.str());

    return *this;
}

//------------------------------------------------------------------------------
//
// Plots a 2d graph from a list of doubles (x) saved in a file
//
Gnuplot& Gnuplot::plotfile_x(const std::string &filename,
                             const unsigned int column,
                             const std::string &title)
{
    //
    // check if file exists
    //
    file_available(filename);


    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //
    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0  &&  two_dim == true)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << "\"" << filename << "\" using " << column;

    if (title == "")
        cmdstr << " notitle ";
    else
        cmdstr << " title \"" << title << "\" ";

    if(smooth == "")
        cmdstr << "with " << pstyle;
    else
        cmdstr << "smooth " << smooth;

    // For file-type plot devices (terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;


}



//------------------------------------------------------------------------------
//
// Plots a 2d graph from a list of doubles (x y) saved in a file
//
Gnuplot& Gnuplot::plotfile_xy(const std::string &filename,
                              const unsigned int column_x,
                              const unsigned int column_y,
                              const std::string &title,
                              const bool auto_range)
{
    //
    // check if file exists
    //
    file_available(filename);


    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //

    if(auto_range)
       cmdstr << "set yrange [*:*]" << std::endl;

    cmd(cmdstr.str());

    cmdstr.str("");

    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0  &&  two_dim == true)
            cmdstr << "replot ";
        else
            cmdstr << "plot ";
    }

    cmdstr << "\"" << filename << "\" using " << column_x << ":" << column_y;

    if (title == "")
        cmdstr << " notitle ";
    else
        cmdstr << " title \"" << title << "\" ";

    if(smooth == "")
        cmdstr << "with " << pstyle;
    else
        cmdstr << "smooth " << smooth;

    // For file-type plot devices (terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("plot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;
}


//------------------------------------------------------------------------------
//
// Plots a 2d graph with errorbars from a list of doubles (x y dy) in a file
//
Gnuplot& Gnuplot::plotfile_xy_err(const std::string &filename,
                                  const unsigned int column_x,
                                  const unsigned int column_y,
                                  const unsigned int column_dy,
                                  const std::string &title)
{
    //
    // check if file exists
    //
    file_available(filename);

    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0  &&  two_dim == true)
        cmdstr << "replot ";
    else
        cmdstr << "plot ";

    cmdstr << "\"" << filename << "\" using "
           << column_x << ":" << column_y << ":" << column_dy
           << " with errorbars ";

    if (title == "")
        cmdstr << " notitle ";
    else
        cmdstr << " title \"" << title << "\" ";

    //
    // Do the actual plot
    //
    cmd(cmdstr.str());

    return *this;
}


//------------------------------------------------------------------------------
//
// Plots a 3d graph from a list of doubles (x y z) saved in a file
//
Gnuplot& Gnuplot::plotfile_xyz(const std::string &filename,
                               const unsigned int column_x,
                               const unsigned int column_y,
                               const unsigned int column_z,
                               const std::string &title)
{
    //
    // check if file exists
    //
    file_available(filename);

    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //
    if(mPlotDevice==WindowPlotDevice)
    {
        if (nplots > 0  &&  two_dim == true)
            cmdstr << "replot ";
        else
            cmdstr << "splot ";
    }

    cmdstr << "\"" << filename << "\" using " << column_x << ":" << column_y
           << ":" << column_z;

    if (title == "")
        cmdstr << " notitle with " << pstyle;
    else
        cmdstr << " title \"" << title << "\" with " << pstyle;

    // For file-type plot devices (terminals) the plot commands are kept on hold
    // and sent to the file all at once (actually when the instance for this session
    // is destroyed). This is because we cannot plot multiple times on a file as we
    // can do on a screen window, unless multiplot mode is set.
    if((mPlotDevice==PngPlotDevice ||
        mPlotDevice==PSPlotDevice) &&
       !mPlotMode==MultiplotMode)
    {
        if(mPlotList.empty())
           mPlotList.push_back("splot "+cmdstr.str());
        else
           mPlotList.push_back(", "+cmdstr.str());
    }
    else
    {
        cmd(cmdstr.str());
    }

    return *this;
}



//------------------------------------------------------------------------------
//
/// *  note that this function is not valid for versions of GNUPlot below 4.2
//
Gnuplot& Gnuplot::plot_image(const unsigned char * ucPicBuf,
                             const unsigned int iWidth,
                             const unsigned int iHeight,
                             const std::string &title)
{
    std::ofstream tmp;
    std::string name = create_tmpfile(tmp);
    if (name == "")
        return *this;

    //
    // write the data to file
    //
    int iIndex = 0;
    for(size_t iRow = 0; iRow < iHeight; iRow++)
    {
        for(size_t iColumn = 0; iColumn < iWidth; iColumn++)
        {
            tmp << iColumn << " " << iRow  << " "
                << static_cast<float>(ucPicBuf[iIndex++]) << std::endl;
        }
    }

    tmp.flush();
    tmp.close();


    std::ostringstream cmdstr;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0  &&  two_dim == true)
        cmdstr << "replot ";
    else
        cmdstr << "plot ";

    if (title == "")
        cmdstr << "\"" << name << "\" with image";
    else
        cmdstr << "\"" << name << "\" title \"" << title << "\" with image";

    //
    // Do the actual plot
    //
    cmd(cmdstr.str());

    return *this;
}



//------------------------------------------------------------------------------
//
// Sends a command to an active gnuplot session
//
Gnuplot& Gnuplot::cmd(const std::string &cmdstr)
{
    if( !is_valid() )
    {
        return *this;
    }

    if(cmdstr.empty())
       return *this;

    // the final command to be sent
    std::string command = cmdstr + "\n";
//std::cout << command;

    // int fputs ( const char * str, FILE * stream );
    // writes the string str to the stream.
    // The function begins copying from the address specified (str) until it
    // reaches the terminating null character ('\0'). This final
    // null-character is not copied to the stream.
    fputs(command.c_str(), gnucmd );

    // int fflush ( FILE * stream );
    // If the given stream was open for writing and the last i/o operation was
    // an output operation, any unwritten data in the output buffer is written
    // to the file.  If the argument is a null pointer, all open files are
    // flushed.  The stream remains open after this call.
    fflush(gnucmd);

    if( cmdstr.find("replot") != std::string::npos )
    {
        return *this;
    }
    else if( cmdstr.find("splot") != std::string::npos )
    {
        two_dim = false;
        nplots++;
    }
    else if( cmdstr.find("plot") != std::string::npos )
    {
        two_dim = true;
        nplots++;
    }

    return *this;
}



