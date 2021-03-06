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
const double cGoldenRatio = (1. + sqrt(5)) / 2.;

double function(double x)
{
    // normal distribution
    const double scale = 1. / sqrt(2 * M_PI);
    return exp(-(x * x / 2.)) * scale;
}

struct _point
{
    _point(double _x, double _y)
        : x(_x), y(_y) {};
    _point() : x(0.), y(0.) {};
    double x;
    double y;
};
typedef _point point;

point parametricFunction(double t)
{
    point result;
    result.x = exp(1/10. * t) * cos(t);
    result.y = exp(1/10. * t) * sin(t);
    return result;
}

struct _size
{
    _size(double _width, double _height)
        : width(_width), height(_height) {};
    double width;
    double height;
};
typedef _size size;

struct _range
{
    _range(double _min, double _max)
        : min(_min), max(_max) {};
    double min;
    double max;
    double getSpan() const { return max - min; }
};
typedef _range range;

struct bezierPoint
{
    point pt[3];
};

enum objectType
{
    OBJECT_CURVES,
    OBJECT_STRAIGHTLINE,
    OBJECT_CLOSEDLINE
};

class baseObject
{
public:
    baseObject(enum objectType _type)
        : type(_type)
        , rangeY(range(0, 0))
        , rangeX(range(0, 0))
    {};
    ~baseObject();
    void drawParametricEquation(point (*func)(double), const range& _rangeT, const int cDivSteps)
    {
        double cStep = _rangeT.getSpan() / cDivSteps;
        //rangeX = _rangeX;

        // for differentials
        bezierPoint dPoint;
        double dStep = cStep / 3.;

        for (size_t i = 0; i <= cDivSteps; i++)
        {
            double t = _rangeT.min + i * cStep;
            point pt = func(t);
            points.push_back(pt);

            // check the range of Y
            rangeX.min = pt.x < rangeX.min ? pt.x : rangeX.min;
            rangeX.max = pt.x > rangeX.max ? pt.x : rangeX.max;
            rangeY.min = pt.y < rangeY.min ? pt.y : rangeY.min;
            rangeY.max = pt.y > rangeY.max ? pt.y : rangeY.max;

            // compute differentials
            point diff[] = { func(t - dStep), func(t + dStep) };
            double slope = (diff[1].y - diff[0].y) / 2.;
            double xStep = (diff[1].x - diff[0].x) / 2.;
            dPoint.pt[1] = point(pt.x - xStep, pt.y - slope);
            dPoint.pt[2] = pt;
            if (i != 0)
            { // skip first iteration
                curves.push_back(dPoint);
            }
            dPoint.pt[0] = point(pt.x + xStep, pt.y + slope);
        }
    }
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
    void drawConnectedLines(double(*func)(double), const range& _rangeX, const range& totalRangeX)
    {
        rangeX = totalRangeX;
        points.push_back(point(_rangeX.min, 0.));

        // From left bottom to left top
        bezierPoint dPoint;
        point Point = point(_rangeX.min, func(_rangeX.min));
        dPoint.pt[2] = Point;
        dPoint.pt[1].x = dPoint.pt[0].x = _rangeX.min;
        dPoint.pt[0].y = Point.y / 3.;
        dPoint.pt[1].y = Point.y * (2./3.);
        curves.push_back(dPoint);
        points.push_back(Point);

        unsigned int cIteration = (unsigned)(_rangeX.getSpan() / 0.1);
        double dDistance = 0.1 / 3.;
        double dStep = 0.1;

        double slope = (func(Point.x + dDistance) - func(Point.x - dDistance)) / 2.;
        for (unsigned i = 0; i < cIteration; i++)
        {
            // curves from left top to right top
            dPoint.pt[0] = point(Point.x + dDistance, Point.y + slope);
            Point.x += dStep;
            Point.y = func(Point.x);
            dPoint.pt[2] = Point;
            slope = (func(Point.x + dDistance) - func(Point.x - dDistance)) / 2.;
            dPoint.pt[1] = point(Point.x - dDistance, Point.y - slope);
            curves.push_back(dPoint);
            points.push_back(Point);
        }

        // line from right top to right bottom
        dPoint.pt[0].x = dPoint.pt[1].x = dPoint.pt[2].x = _rangeX.max;
        dPoint.pt[0].y = Point.y * (2. / 3.);
        dPoint.pt[1].y = Point.y / 3.;
        dPoint.pt[2].y = 0.;
        curves.push_back(dPoint);
        points.push_back(dPoint.pt[2]);

        // Bottom Line
        dPoint.pt[0].y = dPoint.pt[1].y = dPoint.pt[2].y = 0.;
        dPoint.pt[0].x = (_rangeX.max * 2. + _rangeX.min) / 3.;
        dPoint.pt[1].x = (_rangeX.min * 2. + _rangeX.max) / 3.;
        dPoint.pt[2].x = _rangeX.min;
        curves.push_back(dPoint);
        points.push_back(Point);
    }
    void drawArrow(const point& head, const point& tail)
    {
        points.push_back(head);
        points.push_back(tail);
    }
    enum objectType getType() const { return type; }
    range getRangeX() const { return rangeX; }
    range getRangeY() const { return rangeY; }
    void setRangeX(const range& _rangeX) { rangeX = _rangeX; }
    void setRangeY(const range& _rangeY) { rangeY = _rangeY; }
    void drawObject(std::ostream& os, unsigned int counter, const size& _canvasOffset, const size& _canvasSize) const
    {
        int canvasOffsetPPTXWidth = (int)(_canvasOffset.width * cm2pptx);
        int canvasOffsetPPTXHeight = (int)(_canvasOffset.height * cm2pptx);
        int canvasSizePPTXWidth = (int)(_canvasSize.width * cm2pptx);
        int canvasSizePPTXHeight = (int)(_canvasSize.height * cm2pptx);
        switch (type)
        {
        case OBJECT_CLOSEDLINE:
        case OBJECT_CURVES:
        {
            os << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << counter << "\" name=\"qqqqqqqqq\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{29FFCF3D-8F32-482F-B193-ACC1CF50B0FA}\"/></a:ext></a:extLst></p:cNvPr><p:cNvSpPr/><p:nvPr/></p:nvSpPr>";
            os << "<p:spPr><a:xfrm><a:off x=\"" << canvasOffsetPPTXWidth << "\" y=\"" << canvasOffsetPPTXHeight << "\"/>" << std::endl;
            os << "<a:ext cx=\"" << canvasSizePPTXWidth << "\" cy=\"" << canvasSizePPTXHeight << "\"/>" << std::endl << "</a:xfrm><a:custGeom><a:avLst/><a:gdLst>" << std::endl;
            for (size_t i = 0; i < points.size(); i++)
            {
                point canvasPoint = convertToCanvas(points[i], size(canvasSizePPTXWidth, canvasSizePPTXHeight));
                os << "<a:gd name=\"connsiteX" << i << "\"  fmla=\"*/ " << (int)(canvasPoint.x) << " w " << canvasSizePPTXWidth << "\"/>" << std::endl;
                os << "<a:gd name=\"connsiteY" << i << "\"  fmla=\"*/ " << (int)(canvasPoint.y) << " h " << canvasSizePPTXHeight << "\"/>" << std::endl;
            }
            if (type == OBJECT_CLOSEDLINE)
            {
                size_t index = points.size();
                os << "<a:gd name=\"connsiteX" << index << "\"  fmla=\"*/ " << (int)(points[0].x) << " w " << canvasSizePPTXWidth << "\"/>" << std::endl;
                os << "<a:gd name=\"connsiteY" << index << "\"  fmla=\"*/ " << (int)(points[0].y) << " h " << canvasSizePPTXHeight << "\"/>" << std::endl;
            }
            os << "</a:gdLst><a:ahLst/><a:cxnLst>" << std::endl;
            for (size_t i = 0; i < points.size(); i++)
            {
                os << "<a:cxn ang=\"0\">" << std::endl;
                os << "<a:pos x=\"connsiteX" << i << "\" y=\"connsiteY" << i << "\"/>" << std::endl;
                os << "</a:cxn>" << std::endl;
            }
            if (type == OBJECT_CLOSEDLINE)
            {
                size_t index = points.size();
                os << "<a:cxn ang=\"0\">" << std::endl;
                os << "<a:pos x=\"connsiteX" << index << "\" y=\"connsiteY" << index << "\"/>" << std::endl;
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
            if (type == OBJECT_CLOSEDLINE)
            {
                point canvasPoint = convertToCanvas(points[0], size(canvasSizePPTXWidth, canvasSizePPTXHeight));
                os << "<a:lnTo>"
                    << "<a:pt x=\"" << (int)(canvasPoint.x) << "\" y=\"" << (int)(canvasPoint.y) << "\"/>"
                    << "</a:lnTo>"
                    << "<a:lnTo>"
                    << "<a:pt x=\"" << (int)(canvasPoint.x) << "\" y=\"" << (int)(canvasPoint.y) << "\"/>"
                    << "</a:lnTo>"
                    << "<a:close/>";
                os << "</a:path>" << std::endl << "</a:pathLst>" << std::endl << "</a:custGeom><a:solidFill><a:schemeClr val=\"accent1\"/></a:solidFill></p:spPr>" << std::endl;

            }
            else
            {
                os << "</a:path>" << std::endl << "</a:pathLst>" << std::endl << "</a:custGeom><a:noFill/></p:spPr>" << std::endl;
            }
            os << "<p:style><a:lnRef idx=\"2\"><a:schemeClr val=\"accent1\"><a:shade val=\"50000\"/></a:schemeClr></a:lnRef><a:fillRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"lt1\"/></a:fontRef></p:style><p:txBody><a:bodyPr rtlCol=\"0\" anchor=\"ctr\"/><a:lstStyle/><a:p><a:pPr algn=\"ctr\"/><a:endParaRPr kumimoji=\"1\" lang=\"ja-JP\" altLang=\"en-US\"/></a:p></p:txBody></p:sp>";
        }
        break;
        case OBJECT_STRAIGHTLINE:
        {
            double minX = points[0].x < points[1].x ? points[0].x : points[1].x;
            double maxX = points[1].x < points[0].x ? points[0].x : points[1].x;
            double minY = points[0].y < points[1].y ? points[0].y : points[1].y;
            double maxY = points[1].y < points[0].y ? points[0].y : points[1].y;
            point LB = convertToCanvas(point(minX, minY), size(canvasSizePPTXWidth, canvasSizePPTXHeight));
            point RT = convertToCanvas(point(maxX, maxY), size(canvasSizePPTXWidth, canvasSizePPTXHeight));
            int offsetX = (int)(LB.x) + canvasOffsetPPTXWidth;
            int offsetY = (int)(RT.y) + canvasOffsetPPTXHeight;
            int canvasWidth = (int)(((maxX - minX) / (rangeX.max - rangeX.min)) * cm2pptx * _canvasSize.width);
            int canvasHeight = (int)(((maxY - minY) / (rangeY.max - rangeY.min)) * cm2pptx * _canvasSize.height);

            os << "<p:cxnSp><p:nvCxnSpPr><p:cNvPr id=\"" << counter << "\" name=\"LineArrow" << counter << "\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{B606E735-2B9E-47D5-A972-698E782DD359}\"/></a:ext></a:extLst></p:cNvPr><p:cNvCxnSpPr/><p:nvPr/></p:nvCxnSpPr>";
            os << "<p:spPr><a:xfrm flipV=\"1\"><a:off x=\"" << offsetX << "\" y=\"" << offsetY << "\"/><a:ext cx=\"" << canvasWidth << "\" cy=\"" << canvasHeight << "\"/></a:xfrm><a:prstGeom prst=\"straightConnector1\"><a:avLst/></a:prstGeom><a:ln><a:tailEnd type=\"triangle\"/></a:ln></p:spPr><p:style><a:lnRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:lnRef><a:fillRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"tx1\"/></a:fontRef></p:style></p:cxnSp>";
        }
        break;
        default:
            break;
        }
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
        int counter = objectIdCounter;
        for (auto&& it : objects)
        {
            it.drawObject(os, counter++, canvasOffset, canvasSize);
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
        else
        {
            for (auto&& it : objects)
            {
                if (it.getType() == OBJECT_CURVES)
                {
                    objects[objects.size() - 1].setRangeX(rangeX);
                    objects[objects.size() - 1].setRangeY(rangeY);
                }
            }
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
    baseObject mainCurve(OBJECT_CURVES), showRange(OBJECT_CLOSEDLINE);
    mainCurve.drawCurves(function, rangeX, cStepCounts); // main curve of normal distribution
    showRange.drawConnectedLines(function, range(-1., 1.), rangeX);

    range lineSpanY = mainCurve.getRangeY();
    range lineSpanX = mainCurve.getRangeX();
    size marginSize = size(lineSpanX.getSpan() * 0.1, lineSpanY.getSpan() * 0.1);
    baseObject verticalLine(OBJECT_STRAIGHTLINE), horizontalLine(OBJECT_STRAIGHTLINE);

    // vertical arrow of axis
    verticalLine.drawArrow(point(0., lineSpanY.min - marginSize.height), point(0., lineSpanY.max + marginSize.height));
    // horizontal arrow of axis
    horizontalLine.drawArrow(point(cMinRange - marginSize.width, 0.), point(cMaxRange + marginSize.width, 0.));

    normalDistribution.push_back(mainCurve);
    showRange.setRangeY(mainCurve.getRangeY());
    normalDistribution.push_back(showRange);
    normalDistribution.push_back(verticalLine);
    normalDistribution.push_back(horizontalLine);

    std::ofstream ofs("slide1.xml");
    ofs << normalDistribution;

    baseObject logarithmicSpiral(OBJECT_CURVES);
    drawPPTX logarithmicSpiralSlide(size(cCanvasHeight * cGoldenRatio, cCanvasHeight), size(cCanvasOffsetX, cCanvasOffsetY));
    logarithmicSpiral.drawParametricEquation(parametricFunction, range(-6 * M_PI, 2 * M_PI), cStepCounts);
    logarithmicSpiralSlide.push_back(logarithmicSpiral);

    range logRangeX = logarithmicSpiral.getRangeX();
    range logRangeY = logarithmicSpiral.getRangeY();
    marginSize = size(logRangeX.getSpan() * 0.1, logRangeY.getSpan() * 0.1);
    baseObject vLine(OBJECT_STRAIGHTLINE), hLine(OBJECT_STRAIGHTLINE);

    vLine.drawArrow(point(0., logRangeY.min - marginSize.height), point(0., logRangeY.max + marginSize.height));
    hLine.drawArrow(point(logRangeX.min - marginSize.width,  0.), point(logRangeX.max + marginSize.width, 0.));
    logarithmicSpiralSlide.push_back(vLine);
    logarithmicSpiralSlide.push_back(hLine);


    std::ofstream ofs2("slide2.xml");
    ofs2 << logarithmicSpiralSlide;

    return 0;
}
