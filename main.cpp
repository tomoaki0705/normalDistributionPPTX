#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>

const int cm2pptx = 360000;
const int offsetX = 7 * cm2pptx;
const int offsetY = 5 * cm2pptx;
const double cMinRange = -3.0;
const double cMaxRange = 3.0;
const int cCanvasWidth = (int)(cMaxRange - cMinRange) * cm2pptx;
const double cVerticalScale = 8.;

struct _point
{
    double x;
    double y;
};
typedef _point point;

struct bezierPoint
{
    point pt[3];
};

point convertToCanvas(const point& figurePoint, const double minRangeX, const double maxRangeX, const double minRangeY, const double maxRangeY, const int canvasWidth, const int canvasHeight)
{
    point result;
    double ratioX = canvasWidth / (maxRangeX - minRangeX);
    double ratioY = canvasHeight / (maxRangeY - minRangeY);
    result.x = (figurePoint.x - minRangeX) * ratioX;
    result.y = (figurePoint.y - minRangeY) * ratioY;
    result.y = canvasHeight - result.y;
    return result;
}

double function(double x)
{
    // normal distribution
    const double scale = 1. / sqrt(2 * M_PI);
    return exp(-(x * x / 2.)) * scale;
}

int main(int argc, char**argv)
{
    const double cStep = 0.1;
    const int cIteration = (int)((cMaxRange - cMinRange + DBL_EPSILON) / cStep);

    bezierPoint dPoint;
    std::vector<point> points;
    std::vector<bezierPoint> curves;
    double minY = DBL_MAX;
    double maxY = -DBL_MAX;
    for (size_t i = 0; i <= cIteration; i++)
    {
        point pt;
        pt.x = cMinRange + i * cStep;
        pt.y = function(pt.x);
        double dStep = cStep / 3.;
        double slope = (function(pt.x + dStep) - function(pt.x - dStep)) / (dStep * 2);
        double dx = pt.x + dStep;
        points.push_back(pt);
        dPoint.pt[1].x = pt.x - dStep;
        dPoint.pt[1].y = pt.y - slope * dStep;
        dPoint.pt[2].x = pt.x;
        dPoint.pt[2].y = pt.y;
        minY = pt.y < minY ? pt.y : minY;
        maxY = pt.y > maxY ? pt.y : maxY;
        if (i != 0)
        { // skip first iteration
            curves.push_back(dPoint);
        }
        dPoint.pt[0].x = dx;
        dPoint.pt[0].y = pt.y + slope * dStep;
    }
    int canvasHeight = (int)((maxY - minY) * cVerticalScale) * cm2pptx;

    std::ofstream ofs("slide1.xml");
    ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << std::endl;
    ofs << "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"2\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr><p:sp><p:nvSpPr><p:cNvPr id=\"1\" name=\"qqqqqqqqq\"><a:extLst><a:ext uri=\"{FF2B5EF4-FFF2-40B4-BE49-F238E27FC236}\"><a16:creationId xmlns:a16=\"http://schemas.microsoft.com/office/drawing/2014/main\" id=\"{29FFCF3D-8F32-482F-B193-ACC1CF50B0FA}\"/></a:ext></a:extLst></p:cNvPr><p:cNvSpPr/><p:nvPr/></p:nvSpPr>";
    ofs << "<p:spPr><a:xfrm><a:off x=\"967902\" y=\"894933\"/>" << std::endl;
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
    ofs << "<p:style><a:lnRef idx=\"2\"><a:schemeClr val=\"accent1\"><a:shade val=\"50000\"/></a:schemeClr></a:lnRef><a:fillRef idx=\"1\"><a:schemeClr val=\"accent1\"/></a:fillRef><a:effectRef idx=\"0\"><a:schemeClr val=\"accent1\"/></a:effectRef><a:fontRef idx=\"minor\"><a:schemeClr val=\"lt1\"/></a:fontRef></p:style><p:txBody><a:bodyPr rtlCol=\"0\" anchor=\"ctr\"/><a:lstStyle/><a:p><a:pPr algn=\"ctr\"/><a:endParaRPr kumimoji=\"1\" lang=\"ja-JP\" altLang=\"en-US\"/></a:p></p:txBody></p:sp></p:spTree><p:extLst><p:ext uri=\"{BB962C8B-B14F-4D97-AF65-F5344CB8AC3E}\"><p14:creationId xmlns:p14=\"http://schemas.microsoft.com/office/powerpoint/2010/main\" val=\"2550586031\"/></p:ext></p:extLst></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";

    return 0;
}
