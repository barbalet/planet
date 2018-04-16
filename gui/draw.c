/****************************************************************
 
 draw.c
 
 =============================================================
 
 Copyright 1996-2018 Tom Barbalet. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or
 sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 
 This software and Noble Ape are a continuing work of Tom Barbalet,
 begun on 13 June 1996. No apes or cats were harmed in the writing
 of this software.
 
 ****************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "noble.h"
#include "gldraw.h"
#include "planet.h"

#define     GPI_DIMENSION_X 800
#define     GPI_DIMENSION_Y 600

#define		DDC_PI		  	(3.14159265358979323846)

#define		MAXPNTS		 	150000

#define     MULTIPLY        2048
#define     BITSHIFT        11

#define		SCRX	        (GPI_DIMENSION_X<<(BITSHIFT-1))
#define		SCRY	        (GPI_DIMENSION_Y<<(BITSHIFT-1))

#define		RADIUS	     	210
#define		CCUTNUM         6
#define		CCUTNUM2        5

#define		WEATHER_RENDERED
#define		WATER_RENDERED

#define		WATER_LEVEL		0

#define		BIG_DOT			

#define   	PLANET_CD(num)  (sv[((num)+128)&511])
#define  	PLANET_SD(num)  (sv[(num)&511])

#define TURN_PLUS  			1
#define	TURN_MINUS			(512-TURN_PLUS)
#define	CENTRE_BLOCK		43

n_int           pres[16384*2], dl[16384*2];

n_double	    sv[512];

n_double		rm[9]={
	1, 0, 0,
	0, 1, 0,
	0, 0, 1
};

n_int			points[MAXPNTS*3];
n_uint          number_points;
n_uint          number_points_water;


n_int draw_error(n_constant_string error_text, n_constant_string location, n_int line_number)
{
    printf("draw_error %s %s %ld \n", error_text, location, line_number);
    return -1;
}

void gpi_turn(n_double * turn_matrix) {
    n_double   local[9];
    n_byte2    lpy=0;
    while(lpy<9){
        n_byte2 lpx=0;
        while(lpx<3){
            local[ lpx + lpy ]= (turn_matrix[lpy]  *rm[lpx])+
            (turn_matrix[1+lpy]*rm[lpx+3])+
            (turn_matrix[2+lpy]*rm[lpx+6]);
            lpx++;
        }
        lpy += 3;
    }
    lpy = 0;
    while (lpy < 9) {
        rm[lpy] = local[lpy];
        lpy++;
    }
}

n_byte gpi_key(n_byte2 num) {

n_double	turn_matrix[9]={
	1, 0, 0,
	0, 1, 0,
	0, 0, 1
};
n_byte	move=0;
	if(num==28){ /* turn left */
   		turn_matrix[0] = PLANET_CD(TURN_PLUS);
		turn_matrix[1] = PLANET_SD(TURN_PLUS);
		turn_matrix[3] = -PLANET_SD(TURN_PLUS);
		turn_matrix[4] = PLANET_CD(TURN_PLUS);
   		move=1;
	}
	if(num==29){ /* turn right */
		turn_matrix[0] = PLANET_CD(TURN_MINUS);
		turn_matrix[1] = PLANET_SD(TURN_MINUS);
		turn_matrix[3] = -PLANET_SD(TURN_MINUS);
		turn_matrix[4] = PLANET_CD(TURN_MINUS);
		move=1;
	}
	if(num==30){ /* forwards */
		turn_matrix[4] = PLANET_CD(TURN_MINUS);
   		turn_matrix[5] = PLANET_SD(TURN_MINUS);
   		turn_matrix[7] = -PLANET_SD(TURN_MINUS);
   		turn_matrix[8] = PLANET_CD(TURN_MINUS);
		move=1;
	}
	if(num==31){ /* back */
   		turn_matrix[4] = PLANET_CD(TURN_PLUS);
   		turn_matrix[5] = PLANET_SD(TURN_PLUS);
   		turn_matrix[7] = -PLANET_SD(TURN_PLUS);
   		turn_matrix[8] = PLANET_CD(TURN_PLUS);
   		move=1;
	}
	if(move){
        gpi_turn(turn_matrix);
	}
    return(move);
}

n_byte  gpi_mouse(n_int px, n_int py){
    n_int	valx = ( px + py - GPI_DIMENSION_Y );
    n_int   valy = ( px - py );
    n_byte	move = 0;
	if(valx>CENTRE_BLOCK && valy>CENTRE_BLOCK)
   		move = gpi_key(29);
	if(valx<(0-CENTRE_BLOCK) && valy>CENTRE_BLOCK)
   		move = gpi_key(30);
	if(valx<(0-CENTRE_BLOCK) && valy<(0-CENTRE_BLOCK))
  		move = gpi_key(28);
	if(valx>CENTRE_BLOCK && valy<(0-CENTRE_BLOCK))
		move = gpi_key(31);
    return(move);
}


#define	LAND_LONG_END	512
#define	LAND_SHORT_END	256


#define	LAND_LONG_BIT	9
#define	LAND_SHORT_BIT	8


#define	LAND_LONG_END1	(LAND_LONG_END-1)
#define	LAND_SHORT_END1	(LAND_SHORT_END-1)

#define	LAND_LONG_SHORT1 (LAND_LONG_END*LAND_SHORT_END1)

static void roundland_z(n_audio * land_z) {
	n_uint	lpx = 0, lpy = 1;
	n_int	tarr[LAND_LONG_END*2];
    /*	512 x 2 buffer array and first line buffer for final line wrap around	*/
	n_int	temp = 0;

	while (lpx < LAND_LONG_END)
    {
		temp += land_z[lpx++];
    }
	temp = temp >> LAND_LONG_BIT;
	lpx = 0;
	while (lpx < LAND_LONG_END)
    {
		land_z[lpx++] = (n_audio)temp;
    }
	temp = 0;
	lpx = 0;
	while (lpx < LAND_LONG_END)
		temp += land_z[(lpx++) + LAND_LONG_SHORT1];
	temp = temp >> LAND_LONG_BIT;

	lpx = 0;
	while (lpx < LAND_LONG_END) {
		tarr[lpx] = land_z[lpx];
		tarr[lpx + LAND_LONG_END] = land_z[lpx + LAND_LONG_END];
		land_z[lpx + LAND_LONG_SHORT1] = (short)temp;

		lpx++;
	}
	while (lpy < LAND_SHORT_END1) {
		lpx = 0;
		while (lpx < LAND_LONG_END) {
			temp = tarr[lpx] + tarr[lpx + LAND_LONG_END]  /* (0,-1) + (0,0) */
				 + tarr[((lpx + 1) & LAND_LONG_END1) + LAND_LONG_END] + tarr[((lpx + LAND_LONG_END1) & LAND_LONG_END1) + LAND_LONG_END] /* (+1,0) + (-1,0) */
				 + land_z[lpx + ((lpy + 1) << LAND_LONG_BIT)]; /* (0,+1) */
			land_z[lpx + (lpy << LAND_LONG_BIT)] = temp / 5;
			lpx++;
		}
		if (lpy != LAND_SHORT_END1) { /*	advance buffer (unless it is the last time through) */
			lpx = 0;
			while (lpx < LAND_LONG_END) {
				tarr[lpx] = tarr[lpx + LAND_LONG_END];
				tarr[lpx + LAND_LONG_END] = land_z[lpx + ((lpy + 1) << LAND_LONG_BIT)];
				lpx++;
			}
		}
		lpy++;
	}

}

/*	creates a 'fractal' landscape based on the 32-bit value kseed... */

static void draw_point_2d_3d(n_int val3, n_uint px, n_uint py, n_int * pnts){
	n_uint	val1 = (RADIUS + (val3 >> CCUTNUM2));

	n_double sinx = PLANET_SD(px);
	n_double cosx = PLANET_CD(px);

	n_double siny = PLANET_SD(384-py);
	n_double cosy = PLANET_CD(384-py);
    
    pnts[0] = (long)( -(MULTIPLY * val1 * sinx * cosy));
	pnts[1] = (long)( -(MULTIPLY * val1 * cosx * cosy));
	pnts[2] = (long)( -(MULTIPLY * val1 * siny));

}



static void find_dxdy(n_audio * local_land, n_int *local_pres, n_int * local_dl){
n_int	ly = 0;
	while(ly<128){
		n_int lx=0;
		while(lx<256){
			n_uint	offset = (lx<<1)+(ly<<10);
			n_int			tmp = local_land[ offset ] + local_land[ 1 + offset ] + local_land[ 512 + offset ] + local_land[ 513 + offset ];
			local_pres[lx+(ly<<8)] = tmp >> 2;
			lx++;
		}
		ly++;
	}
	
	ly=0;
	while(ly<128){	
		n_int	lx=0;
		n_uint	yoffset = (ly<<8);
		n_uint	yoffsetp1 = (((ly+1)&127)<<8);
		n_uint	yoffsetm1 = (((ly+127)&127)<<8);
		while(lx<256){
			local_dl[lx+yoffset] = local_pres[((lx+1)&255)+yoffset]-local_pres[((lx+255)&255)+yoffset]+local_pres[lx+yoffsetp1]-local_pres[lx+yoffsetm1];
			lx++;
		}
		ly++;
	}
	
	ly=0;
	while(ly<(16384*2)){
		local_pres[ly++]=0;
	}
}

#define	BINARY_NUMBER	3


n_uint	landtime = 0;

static void	normalise_pressure(n_int *local_pres){
	n_double	value = 0E+00;
	n_int	loop = 0;
	n_int	diff;
	while(loop < (16384*2 ) ){
		n_double	doub_pres = local_pres[loop++]/1E+00;
		value += doub_pres;
	}
	diff = (n_int)(value/(16384*2));
	loop = 0;
	while(loop < (16384*2 ) ){
		local_pres[loop++] -= diff;
	}
}

static void deetee(n_int * local_pres, n_int * local_dl){
n_int	lx = 0;
	while(lx<256){
		n_int ly = 0;
		n_int	tmp3 = ((lx+255)&255);
		n_int	tmp4 = ((lx+1)&255);
		while(ly<128){
			n_int	tmp2 = ly<<8;
			n_int	tmp  = lx|tmp2;
			local_pres[tmp] += ( local_dl[tmp] 
							 - local_pres[tmp4|tmp2] 
				             + local_pres[tmp3|tmp2]
				             - local_pres[lx|(((ly+1)&127)<<8)]
				             + local_pres[lx|(((ly+127)&127)<<8)]
			             )>>7;
			ly++;
		}
		lx++;
	}
	normalise_pressure(local_pres);
	landtime++;
}



#define	MAGIC_NUMBER	(15)

n_int			weather_points[MAXPNTS*3];
n_uint	num_weather_points = 0;

static void	draw_weather(long * local_pres){
n_int scr_y=0;
	num_weather_points = 0;
	while(scr_y<128){
		n_int scr_ym=scr_y<<1;
		n_int scr_x=0;
		while(scr_x<256){
			n_int scr_xm=scr_x<<1;
			n_int  tmp_long=local_pres[scr_x+(scr_y<<8)];
			
			if(tmp_long > 0){
				if(num_weather_points < MAXPNTS){
					draw_point_2d_3d(300, scr_xm,scr_ym, &weather_points[(num_weather_points*3)]);
					num_weather_points++;
				}
				if(num_weather_points < MAXPNTS){
					draw_point_2d_3d(300, scr_xm+1,scr_ym, &weather_points[(num_weather_points*3)]);
					num_weather_points++;
				}
				if(num_weather_points < MAXPNTS){
					draw_point_2d_3d(300, scr_xm+1,scr_ym+1, &weather_points[(num_weather_points*3)]);
					num_weather_points++;
				}
				if(num_weather_points < MAXPNTS){
					draw_point_2d_3d(300, scr_xm,scr_ym+1, &weather_points[(num_weather_points*3)]);
					num_weather_points++;
				}
			}
			scr_x++;
		}
		scr_y++;
	}
}

long gpi_init(unsigned long kseed){
	n_byte	refine = 0;
	n_uint 	major = 0, minor = 0;
	n_uint	tx, ty, mx, my, px, py;
	n_uint	tseed = kseed;
    n_audio	land_z[131072];
  
	while (major < 512) {
		sv[major] = sin((n_double)(DDC_PI * major) / 256);
		major++;
	}
    while (minor < 131072) {
		land_z[minor++] = 0;
    }
	while (refine < 7) {/*	spread 32-bits incremented down over 7 halving iterations... */
		major = 64 >> refine;
		minor = 1 << refine;
		py = 0;
		while (py < minor) {
			px = 0;
			while (px < minor) {
				n_uint val1 = (px << 3) + (py << 11);
				tseed *= 1103515245;
				tseed += 12345;
				ty = 0;
				while (ty < 4) {
					tx = 0;
					while (tx < 8) {
						n_uint val2 = tseed >> (tx | (ty << 3));
						n_int 		  val3 = (((val2 & 1) << 1)-1) << CCUTNUM;
						val2 = tx | (ty << 9);
						my = 0;
						while (my < major) {
							mx = 0;
							while (mx < major) {
								land_z[mx + (my << 9) + (major*(val1 + val2))] += (short)val3;
								mx++;
							}
							my++;
						}
						tx++;
					}
					ty++;
				}
				px++;
			}
			py++;
		}

		roundland_z(land_z);
		roundland_z(land_z);
		roundland_z(land_z);
		roundland_z(land_z);
		roundland_z(land_z);
		roundland_z(land_z);

		refine++;
	}
	number_points = 0;
	py = 0;
	while (py < 255) {
		px = 0;
		while (px < 512) {
			n_uint val2 = px + (py << 9);
			n_int 		  val3 = land_z[val2];
			refine = 255;

			if ((val3 >> CCUTNUM) != (land_z[val2 + 1] >> CCUTNUM))
				refine = 0;
			if ((val3 >> CCUTNUM) != (land_z[val2 + 512] >> CCUTNUM))
				refine = 0;
			if (refine == 0 && number_points < MAXPNTS) {
				draw_point_2d_3d(val3, px, py, &points[(number_points*3)]);
				number_points++;
			}
			px++;
		}
		py++;
	}
    
    number_points_water = 0;
    py = 0;
    while (py < 255) {
        px = 0;
        while (px < 512) {
            n_uint val2 = px + (py << 9);
            n_int           val3 = land_z[val2];
            refine = 255;

#ifdef WATER_RENDERED
            if((val3 <= WATER_LEVEL) && ((px&1) == 0)&&((py&1) == 0))
            {
                refine = 1;
            }
            if (refine == 1 && (number_points + number_points_water) < MAXPNTS) {
                draw_point_2d_3d(0, px, py, &points[((number_points + number_points_water)*3)]);
                number_points_water++;
            }
#endif
            px++;
        }
        py++;
    }
	find_dxdy(land_z, pres, dl);
	draw_weather(pres);
    return 1;
}


static void	draw_3d_points(n_int * pnts, n_uint loopend){
  	n_uint	 loop = 0;
  	n_int  a0=(long)(MULTIPLY*rm[0]);
  	n_int  a1=(long)(MULTIPLY*rm[1]);
  	n_int  a2=(long)(MULTIPLY*rm[2]);
  	n_int  a3=(long)(MULTIPLY*rm[3]);
  	n_int  a4=(long)(MULTIPLY*rm[4]);
  	n_int  a5=(long)(MULTIPLY*rm[5]);
  	n_int  a6=(long)(MULTIPLY*rm[6]);
  	n_int  a7=(long)(MULTIPLY*rm[7]);
  	n_int  a8=(long)(MULTIPLY*rm[8]);
	
	loopend = loopend * 3;
	while (loop < loopend) {
		n_int	act_x = pnts[ loop++ ];
		n_int	act_y = pnts[ loop++ ];
		n_int	act_z = pnts[ loop++ ];
		
		n_int	scr_z = ((a6 * act_x) + (a7 * act_y) + (a8 * act_z))>>BITSHIFT;
		if ( scr_z > 0) {
            n_vect2 scr;
			scr_z = MULTIPLY + (scr_z>>(BITSHIFT+3));			
			act_x = (act_x * scr_z)>> BITSHIFT;
			act_y = (act_y * scr_z)>> BITSHIFT;
			act_z = (act_z * scr_z)>> BITSHIFT;
			scr.x = (((a0 * act_x) + (a1 * act_y) + (a2 * act_z) )>>(BITSHIFT*2)) + (SCRX>>BITSHIFT);
			scr.y = (((a3 * act_x) + (a4 * act_y) + (a5 * act_z) )>>(BITSHIFT*2)) + (SCRY>>BITSHIFT);
            
            gldraw_vertex(&scr);
            
            scr.x -= 1;
            gldraw_vertex(&scr);
            scr.x += 2;
            gldraw_vertex(&scr);
            scr.x -= 1;
            scr.y -= 1;
            gldraw_vertex(&scr);
            scr.y += 2;
            gldraw_vertex(&scr);
		}
	}

}


void   gpi_cycle(void){
    gldraw_background_black();
    
    gldraw_start_points();
    gldraw_grey();
    draw_3d_points( points , number_points );
#ifdef WATER_RENDERED
    gldraw_blue_clear();
    draw_3d_points( &points[number_points*3] , number_points_water );
#endif
#ifdef WEATHER_RENDERED
    deetee(pres,dl);
    if((landtime&BINARY_NUMBER)==0){
        draw_weather(pres);
    }
    gldraw_lightgrey_clear();
    draw_3d_points( weather_points , num_weather_points );
#endif
    gldraw_end_points();
}

