#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <cfloat>
#include <fstream>
#include <vector>

const int cm2pptx = 360000;
const int cStepCounts = 100;
const double cCanvasOffsetX = 7;
const double cCanvasOffsetY = 5;
const double cCanvasWidth = 12;
const double cCanvasHeight = 9;
const double cMinRange = -3.0;
const double cMaxRange = 3.0;

double function(double x)
{
    // normal distribution
    const double scale = 1. / sqrt(2 * M_PI);
    return exp(-(x * x / 2.)) * scale;
}

struct _point
{
    struct _point(double _x, double _y)
        : x(_x), y(_y) {};
    struct _point() : x(0.), y(0.) {};
    double x;
    double y;
};
typedef _point point;

struct _size
{
    struct _size(double _width, double _height)
        : width(_width), height(_height) {};
    double width;
    double height;
};
typedef _size size;

struct _range
{
    struct _range(double _min, double _max)
        : min(_min), max(_max) {};
    double min;
    double max;
};
typedef _range range;

struct bezierPoint
{
    point pt[3];
};

enum objectType
{
    OBJECT_CURVES,
    OBJECT_STRAIGHTLINE
};

class baseObject
{
public:
    baseObject(enum objectType _type)
        : type(_type)
        , rangeY(range(DBL_MAX, -DBL_MAX))
        , rangeX(range(DBL_MAX, -DBL_MAX))
    {};
    ~baseObject();
    void drawCurves(double(*func)(double), const range& _rangeX, const int cDivSteps)
    {
        rangeX = _rangeX;
        double cStep = (rangeX.max - rangeX.min) / cDivSteps;

        // for differentials
        bezierPoint dPoint;
        double dStep = cStep / 3.;

        for (size_t i = 0; i <= cDivSteps; i++)
        {
            double x = rangeX.min + i * cStep;
            point pt(x, func(x));
            points.push_back(pt);

            // check the range of Y
            rangeY.min = pt.y < rangeY.min ? pt.y : rangeY.min;
            rangeY.max = pt.y > rangeY.max ? pt.y : rangeY.max;

            // compute differentials
            double slope = (func(pt.x + dStep) - func(pt.x - dStep)) / 2;
            dPoint.pt[1] = point(pt.x - dStep, pt.y - slope);
            dPoint.pt[2] = pt;
            if (i != 0)
            { // skip first iteration
                curves.push_back(dPoint);
            }
            dPoint.pt[0] = point(pt.x + dStep, pt.y + slope);
        }
    }
    enum objectType getType() const { return type; }
    range getRangeX() const { return rangeX; }
    range getRangeY() const { return rangeY; }
    void drawObject(std::ostream& os, unsigned int counter, const size& _canvasOffset, const size& _canvasSize) const
    {
        int canvasOffsetPPTXWidth = (int)(_canvasOffset.width * cm2pptx);
        int canvasOffsetPPTXHeight = (int)(_canvasOffset.height * cm2pptx);
        int canvasSizePPTXWidth = (int)(_canvasSize.width * cm2pptx);
        int canvasSizePPTXHeight = (int)(_canvasSize.height * cm2pptx);
        os << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << counter << "\" name=\"qqqqqqqqq\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{29FFCF3D-8F32-482F-B193-ACC1CF50B0FA}\"/></a:ext></a:extLst></p:cNvPr><p:cNvSpPr/><p:nvPr/></p:nvSpPr>";
        os << "<p:spPr><a:xfrm><a:off x=\"" << canvasOffsetPPTXWidth << "\" y=\"" << canvasOffsetPPTXHeight << "\"/>" << std::endl;
        os << "<a:ext cx=\"" << canvasSizePPTXWidth << "\" cy=\"" << canvasSizePPTXHeight << "\"/>" << std::endl << "</a:xfrm><a:custGeom><a:avLst/><a:gdLst>" << std::endl;
        for (size_t i = 0; i < points.size(); i++)
        {
            point canvasPoint = convertToCanvas(points[i], size(canvasSizePPTXWidth, canvasSizePPTXHeight));
            os << "<a:gd name=\"connsiteX" << i << "\"  fmla=\"*/ " << (int)(canvasPoint.x) << " w " << canvasSizePPTXWidth  << "\"/>" << std::endl;
            os << "<a:gd name=\"connsiteY" << i << "\"  fmla=\"*/ " << (int)(canvasPoint.y) << " h " << canvasSizePPTXHeight << "\"/>" << std::endl;
        }
        os << "</a:gdLst><a:ahLst/><a:cxnLst>" << std::endl;
        for (size_t i = 0; i < points.size(); i++)
        {
            os << "<a:cxn ang=\"0\">" << std::endl;
            os << "<a:pos x=\"connsiteX" << i << "\" y=\"connsiteY" << i << "\"/>" << std::endl;
            os << "</a:cxn>" << std::endl;
        }
        {
            point canvasPointStart = convertToCanvas(points[0], size(canvasSizePPTXWidth, canvasSizePPTXHeight));
            os << "</a:cxnLst><a:rect l=\"l\" t=\"t\" r=\"r\" b=\"b\"/><a:pathLst>" << std::endl;
            os << "<a:path w=\"" << canvasSizePPTXWidth << "\" h=\"" << canvasSizePPTXHeight << "\">" << std::endl;
            os << "<a:moveTo>" << std::endl;
            os << "<a:pt x=\"" << (int)(canvasPointStart.x) << "\" y=\"" << (int)(canvasPointStart.y) << "\"/>" << std::endl;
            os << "</a:moveTo>" << std::endl;
        }
        for (auto&& it : curves)
        {
            os << "<a:cubicBezTo>" << std::endl;
            for (size_t i = 0; i < 3; i++)
            {
                point canvasPoint = convertToCanvas(it.pt[i], size(canvasSizePPTXWidth, canvasSizePPTXHeight));
                os << "<a:pt x=\"" << (int)(canvasPoint.x) << "\" y=\"" << (int)(canvasPoint.y) << "\"/>" << std::endl;
            }
            os << "</a:cubicBezTo>" << std::endl;
        }
        os << "</a:path>" << std::endl << "</a:pathLst>" << std::endl << "</a:custGeom><a:noFill/></p:spPr>" << std::endl;
        os << "<p:style><a:lnRef idx=\"2\"><a:schemeClr val=\"accent1\"><a:shade val=\"50000\"/></a:schemeClr></a:lnRef><a:fillRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"lt1\"/></a:fontRef></p:style><p:txBody><a:bodyPr rtlCol=\"0\" anchor=\"ctr\"/><a:lstStyle/><a:p><a:pPr algn=\"ctr\"/><a:endParaRPr kumimoji=\"1\" lang=\"ja-JP\" altLang=\"en-US\"/></a:p></p:txBody></p:sp>";
    }

private:
    point convertToCanvas(const point& figurePoint, const size& canvasSize) const
    {
        point result;
        double ratioX = canvasSize.width / (rangeX.max - rangeX.min);
        double ratioY = canvasSize.height / (rangeY.max - rangeY.min);
        result.x = (figurePoint.x - rangeX.min) * ratioX;
        result.y = (figurePoint.y - rangeY.min) * ratioY;
        result.y = canvasSize.height - result.y;
        return result;
    }


    range rangeX;
    range rangeY;
    enum objectType type;
    std::vector<point> points;
    std::vector<bezierPoint> curves;
};

baseObject::~baseObject()
{
}

class drawPPTX
{
public:
    drawPPTX(const size& _canvasSize, const size& _canvasOffset)
        : objectIdCounter(2)
        , rangeX(range(cMinRange, cMaxRange))
        , rangeY(range(0.0, 1.0))
        , canvasOffset(_canvasOffset)
        , canvasSize(_canvasSize)
    {};
    ~drawPPTX() {} ;
    void drawHeader(std::ostream& os) const
    {
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << std::endl;
        os << "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:cSld><p:spTree>";
        os << "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>";
    }
    void drawFooter(std::ostream& os) const
    {
        os << "</p:spTree><p:extLst><p:ext uri=\"{BB962C8B-B14F-4D97-AF65-F5344CB8AC3E}\"><p14:creationId xmlns:p14=\"http://schemas.microsoft.com/office/powerpoint/2010/main\" val=\"2550586031\"/></p:ext></p:extLst></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";
    }
    void drawObjects(std::ostream& os) const
    {
        for (auto&& it : objects)
        {
            switch (it.getType())
            {
            case OBJECT_CURVES:
                it.drawObject(os, objectIdCounter, canvasOffset, canvasSize);
                break;
            case OBJECT_STRAIGHTLINE:
            default:
                break;
            }
        }
    }
    void push_back(const baseObject& obj)
    {
        objects.push_back(obj);
        if (obj.getType() == OBJECT_CURVES)
        {
            rangeX = obj.getRangeX();
            rangeY = obj.getRangeY();
        }
    }

private:
    std::vector<baseObject> objects;
    size canvasSize, canvasOffset;
    unsigned int objectIdCounter;
    range rangeX, rangeY;
};

std::ostream& operator << (std::ostream& os, const drawPPTX& a)
{
    // output header
    a.drawHeader(os);
    a.drawObjects(os);
    a.drawFooter(os);
    return os;
}

int main(int argc, char**argv)
{
    drawPPTX normalDistribution(size(cCanvasWidth, cCanvasHeight), size(cCanvasOffsetX, cCanvasOffsetY));

    range rangeX(cMinRange, cMaxRange);
    baseObject mainCurve(OBJECT_CURVES);
    mainCurve.drawCurves(function, rangeX, cStepCounts);

    normalDistribution.push_back(mainCurve);

    std::ofstream ofs("slide1.xml");
    ofs << normalDistribution;
#if 0
    ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << std::endl;
    ofs << "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>";
    ofs << "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"qqqqqqqqq\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{29FFCF3D-8F32-482F-B193-ACC1CF50B0FA}\"/></a:ext></a:extLst></p:cNvPr><p:cNvSpPr/><p:nvPr/></p:nvSpPr>";
    ofs << "<p:spPr><a:xfrm><a:off x=\"" << offsetX << "\" y=\"" << offsetY << "\"/>" << std::endl;
    ofs << "<a:ext cx=\"" << cCanvasWidth << "\" cy=\"" << canvasHeight << "\"/>" << std::endl << "</a:xfrm><a:custGeom><a:avLst/><a:gdLst>" << std::endl;
    int index = 0;
    for (auto&& it : points)
    {
        point canvasPoint = convertToCanvas(it, cMinRange, cMaxRange, minY, maxY, cCanvasWidth, canvasHeight);
        ofs << "<a:gd name=\"connsiteX" << index << "\"  fmla=\"*/ " << (int)(canvasPoint.x) << " w " << cCanvasWidth << "\"/>" << std::endl;
        ofs << "<a:gd name=\"connsiteY" << index << "\"  fmla=\"*/ " << (int)(canvasPoint.y) << " h " << canvasHeight << "\"/>" << std::endl;
        index++;
    }
    index = 0;
    ofs << "</a:gdLst><a:ahLst/><a:cxnLst>" << std::endl;
    for (auto&& it : points)
    {
        ofs << "<a:cxn ang=\"0\">" << std::endl;
        ofs << "<a:pos x=\"connsiteX" << index << "\" y=\"connsiteY" << index << "\"/>" << std::endl;
        ofs << "</a:cxn>" << std::endl;
        index++;
    }
    {
        point canvasPointStart = convertToCanvas(points[0], cMinRange, cMaxRange, minY, maxY, cCanvasWidth, canvasHeight);
        ofs << "</a:cxnLst><a:rect l=\"l\" t=\"t\" r=\"r\" b=\"b\"/><a:pathLst>" << std::endl;
        ofs << "<a:path w=\"" << cCanvasWidth << "\" h=\"" << canvasHeight << "\">" << std::endl;
        ofs << "<a:moveTo>" << std::endl;
        ofs << "<a:pt x=\"" << (int)(canvasPointStart.x) << "\" y=\"" << (int)(canvasPointStart.y) << "\"/>" << std::endl;
        ofs << "</a:moveTo>" << std::endl;
    }
    index = 0;
    for (auto&& it : curves)
    {
        ofs << "<a:cubicBezTo>" << std::endl;
        for (size_t i = 0; i < 3; i++)
        {
            point canvasPoint = convertToCanvas(it.pt[i], cMinRange, cMaxRange, minY, maxY, cCanvasWidth, canvasHeight);
            ofs << "<a:pt x=\"" << (int)(canvasPoint.x) << "\" y=\"" << (int)(canvasPoint.y) << "\"/>" << std::endl;
        }
        ofs << "</a:cubicBezTo>" << std::endl;
    }
    ofs << "</a:path>" << std::endl << "</a:pathLst>" << std::endl << "</a:custGeom><a:noFill/></p:spPr>" << std::endl;
    ofs << "<p:style><a:lnRef idx=\"2\"><a:schemeClr val=\"accent1\"><a:shade val=\"50000\"/></a:schemeClr></a:lnRef><a:fillRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"lt1\"/></a:fontRef></p:style><p:txBody><a:bodyPr rtlCol=\"0\" anchor=\"ctr\"/><a:lstStyle/><a:p><a:pPr algn=\"ctr\"/><a:endParaRPr kumimoji=\"1\" lang=\"ja-JP\" altLang=\"en-US\"/></a:p></p:txBody></p:sp>";
    // start showing vertical arrow line
    ofs << "<p:cxnSp><p:nvCxnSpPr><p:cNvPr id=\"4\" name=\"LineArrow3\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{B606E735-2B9E-47D5-A972-698E782DD359}\"/></a:ext></a:extLst></p:cNvPr><p:cNvCxnSpPr/><p:nvPr/></p:nvCxnSpPr>";
    ofs << "<p:spPr><a:xfrm flipV=\"1\"><a:off x=\"" << offsetX + (cCanvasWidth / 2) << "\" y=\"" << (int)(offsetY - 0.1 * canvasHeight) << "\"/><a:ext cx=\"0\" cy=\"" << (int)(canvasHeight * 1.2) << "\"/></a:xfrm><a:prstGeom prst=\"straightConnector1\"><a:avLst/></a:prstGeom><a:ln><a:tailEnd type=\"triangle\"/></a:ln></p:spPr><p:style><a:lnRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:lnRef><a:fillRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"tx1\"/></a:fontRef></p:style></p:cxnSp>";
    // start showing horizontal arrow line
    ofs << "<p:cxnSp><p:nvCxnSpPr><p:cNvPr id=\"5\" name=\"LineArrow4\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{B606E735-2B9E-47D5-A972-698E782DD359}\"/></a:ext></a:extLst></p:cNvPr><p:cNvCxnSpPr/><p:nvPr/></p:nvCxnSpPr>";
    ofs << "<p:spPr><a:xfrm flipV=\"1\"><a:off x=\"" << (int)(offsetX - (cCanvasWidth * 0.1)) << "\" y=\"" << (int)(offsetY + convertToCanvasY(0., maxY, minY, canvasHeight)) << "\"/><a:ext cx=\"" << (int)(cCanvasWidth * 1.2) << "\" cy=\"0\"/></a:xfrm><a:prstGeom prst=\"straightConnector1\"><a:avLst/></a:prstGeom><a:ln><a:tailEnd type=\"triangle\"/></a:ln></p:spPr><p:style><a:lnRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:lnRef><a:fillRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"tx1\"/></a:fontRef></p:style></p:cxnSp>";
    ofs << "</p:spTree><p:extLst><p:ext uri=\"{BB962C8B-B14F-4D97-AF65-F5344CB8AC3E}\"><p14:creationId xmlns:p14=\"http://schemas.microsoft.com/office/powerpoint/2010/main\" val=\"2550586031\"/></p:ext></p:extLst></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";
#endif

    return 0;
}
