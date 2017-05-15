#include <lib/gui/egauge.h>

// No math lib !! We have to use own local functions
double nab_factorial_div( double value, int x);
double nab_powerd( double x, int y);
double nab_SIN( double x);
double nab_COS(double x);
double nab_Radians( double number );


eGauge::eGauge(eWidget *parent)
	:eWidget(parent), m_have_border_color(false)
{

}

void eGauge::setBorderColor(const gRGB &color)
{
	m_border_color=color;
	m_have_border_color=true;
	invalidate();
}

int eGauge::event(int event, void *data, void *data2)
{
	switch (event)
	{
	case evtPaint:
	{
		ePtr<eWindowStyle> style;

		eSize s(size());
//		getStyle(style);
			/* paint background */
//		eWidget::event(evtPaint, data, data2);

		gPainter &painter = *(gPainter*)data2;
//		style->setStyle(painter, eWindowStyle::styleLabel); // TODO - own style

		gRGB pal[256];
		pal[0] = 0;
		pal[1] = 0xff0000;
		pal[2] = 0xffFFff;
		pal[3] = 0x00ff00;
	
		for (int a=0; a<0x10; ++a)
			pal[a | 0x10] = (0x111111 * a) | 0xFF;
		painter.setPalette(pal, 0, 256);

		if (m_have_border_color) {
			painter.setBackgroundColor(gColor(1));
			painter.setForegroundColor(gColor(1));
		} else  {
			painter.setBackgroundColor(gColor(2));
			painter.setForegroundColor(gColor(2));
		}

		painter.line(ePoint(basex, basey), ePoint(endx, endy));
		painter.line(ePoint(basex, (basey -1)), ePoint(endx, endy));
		painter.line(ePoint(basex, (basey +1)), ePoint(endx, endy));
		painter.line(ePoint((basex -1), basey), ePoint(endx, endy));
		painter.line(ePoint((basex +1), basey), ePoint(endx, endy));
		if(basex < endx)
			painter.line(ePoint(basex, basey), ePoint((endx -1), endy));
		if(basex > endx)
			painter.line(ePoint(basex, basey), ePoint((endx +1), endy));
		if(basey > endy)
			painter.line(ePoint(basex, basey), ePoint(endx, (endy -1)));
		if(basey < endy)
			painter.line(ePoint(basex, basey), ePoint(endx, (endy +1)));


		return 0;
	}
	case evtChangedGauge:
	{
		
		int mystart = 0;
		int perc = m_value;

		basex = size().width() >> 1;
		basey = size().height() >> 1;
		double angle = (double) mystart + (double) perc * (double)(360 - (mystart<<1)) / 100.0;
		double rads  = nab_Radians(angle);
		
		endx = basex + (int) (nab_SIN(rads) * (double)(size().width())/2.0);
		endy = basey - (int) (nab_COS(rads) * (double)(size().height())/2.0);

		invalidate();
		
		return 0;
	}
	default:
		return eWidget::event(event, data, data2);
	}
}

void eGauge::setValue(int value)
{
	m_value = value;
	event(evtChangedGauge);
}

// Local Math functions missed in libs
/*----------------------------------------------------------------------------*/
double nab_factorial_div( double value, int x)
{
	if(!x)
		return 1;
	else
	{
		while( x > 1)
		{
			value = value / x--;
		}
	}
	return value;
}

double nab_powerd( double x, int y)
{
	int i=0;
	double ans=1.0;

	if(!y)
		return 1.000;
	else
	{
		while( i < y)
		{
			i++;
			ans = ans * x;
		}
	}
	return ans;
}

double nab_SIN( double x)
{
	int i=0;
	int j=1;
	int sign=1;
	double y1 = 0.0;
	double diff = 1000.0;

	if (x < 0.0)
	{
		x = -1 * x;
		sign = -1;
	}

	while ( x > 360.0*M_PI/180)
	{
		x = x - 360*M_PI/180;
	}

	if( x > (270.0 * M_PI / 180) )
	{
		sign = sign * -1;
		x = 360.0*M_PI/180 - x;
	}
	else if ( x > (180.0 * M_PI / 180) )
	{
		sign = sign * -1;
		x = x - 180.0 *M_PI / 180;
	}
	else if ( x > (90.0 * M_PI / 180) )
	{
		x = 180.0 *M_PI / 180 - x;
	}

	while( nab_powerd( diff, 2) > 1.0E-16 )
	{
		i++;
		diff = j * nab_factorial_div( nab_powerd( x, (2*i -1)) ,(2*i -1));
		y1 = y1 + diff;
		j = -1 * j;
	}
	return ( sign * y1 );
}

double nab_COS(double x)
{
	return nab_SIN(90 * M_PI / 180 - x);
}

double nab_Radians( double number )
{
	return number*M_PI/180;
}