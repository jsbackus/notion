/*
 * ion/ioncore/sizepolicy.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/minmax.h>
#include <string.h>

#include "common.h"
#include "region.h"
#include "resize.h"
#include "sizehint.h"
#include "sizepolicy.h"



static int fit_x(int x, int w, const WRectangle *max_geom)
{
    int mw=maxof(max_geom->w, 1);
    w=minof(mw, w);
    return minof(maxof(x, max_geom->x), max_geom->x+mw-w);
}


static int fit_y(int y, int h, const WRectangle *max_geom)
{
    int mh=maxof(max_geom->h, 1);
    h=minof(mh, h);
    return minof(maxof(y, max_geom->y), max_geom->y+mh-h);
}


static void do_gravity(const WRectangle *max_geom, int szplcy,
                       WRectangle *geom)
{
    /* Assumed: width and height already adjusted within limits */
    if(geom->h<1)
        geom->h=1;
    if(geom->w<1)
        geom->w=1;
    
    switch(szplcy&SIZEPOLICY_HORIZ_MASK){
    case SIZEPOLICY_HORIZ_LEFT:
        geom->x=max_geom->x;
        break;
        
    case SIZEPOLICY_HORIZ_RIGHT:
        geom->x=max_geom->x+max_geom->w-geom->w;
        break;

    case SIZEPOLICY_HORIZ_CENTER:
        geom->x=max_geom->x+max_geom->w/2-geom->w/2;
        break;
        
    default:
        geom->x=fit_x(geom->x, geom->w, max_geom);
    }

    switch(szplcy&SIZEPOLICY_VERT_MASK){
    case SIZEPOLICY_VERT_TOP:
        geom->y=max_geom->y;
        break;
        
    case SIZEPOLICY_VERT_BOTTOM:
        geom->y=max_geom->y+max_geom->h-geom->h;
        break;

    case SIZEPOLICY_VERT_CENTER:
        geom->y=max_geom->y+max_geom->h/2-geom->h/2;
        break;
        
    default:
        geom->y=fit_x(geom->y, geom->h, max_geom);
    }
}


static void correct_size(WRegion *reg, int *w, int *h)
{
    XSizeHints hints;
    
    if(reg!=NULL){
        region_size_hints(reg, &hints);
        xsizehints_correct(&hints, w, h, FALSE);
    }
}


static void gravity_stretch_policy(int szplcy, WRegion *reg,
                                   const WRectangle *rq_geom, WFitParams *fp, 
                                   bool ws, bool hs)
{
    WRectangle max_geom=fp->g;
    int w, h;

    fp->g=*rq_geom;
    
    w=(ws ? max_geom.w : minof(rq_geom->w, max_geom.w));
    h=(hs ? max_geom.h : minof(rq_geom->h, max_geom.h));
    
    if(rq_geom->w!=w || rq_geom->h!=h){
        /* Only apply size hint correction if we altered the 
         * requested geometry.
         */
        correct_size(reg, &w, &h);
    }
    
    fp->g.w=w;
    fp->g.h=h;

    do_gravity(&max_geom, szplcy, &(fp->g));
    
    fp->mode=REGION_FIT_EXACT;
}


static void sizepolicy_free_snap(WSizePolicy *szplcy, WRegion *reg,
                                 WRectangle *rq_geom, int rq_flags,
                                 WFitParams *fp)
{
    WRectangle max_geom=fp->g;
    bool fullw=((rq_flags&REGION_RQGEOM_WEAK_W) &&
                (*szplcy&SIZEPOLICY_HORIZ_MASK)==SIZEPOLICY_HORIZ_CENTER);
    bool fullh=((rq_flags&REGION_RQGEOM_WEAK_H) &&
                (*szplcy&SIZEPOLICY_VERT_MASK)==SIZEPOLICY_VERT_CENTER);

    int w=(fullw ? max_geom.w : minof(rq_geom->w, max_geom.w));
    int h=(fullh ? max_geom.h : minof(rq_geom->h, max_geom.h));
    int x_=0, y_=0;

    
    if(!(rq_flags&REGION_RQGEOM_WEAK_X) 
       && rq_flags&REGION_RQGEOM_WEAK_W){
        x_=fit_x(rq_geom->x, 1, &max_geom);
        if(((*szplcy)&SIZEPOLICY_HORIZ_MASK)==SIZEPOLICY_HORIZ_RIGHT)
            w=max_geom.x+max_geom.w-x_;
        else
            w=minof(w, max_geom.x+max_geom.w-x_);
    }
    
    if(!(rq_flags&REGION_RQGEOM_WEAK_Y)
       && rq_flags&REGION_RQGEOM_WEAK_H){
        y_=fit_x(rq_geom->y, 1, &max_geom);
        if(((*szplcy)&SIZEPOLICY_VERT_MASK)==SIZEPOLICY_VERT_BOTTOM)
            h=max_geom.y+max_geom.h-y_;
        else
            h=minof(h, max_geom.y+max_geom.h-y_);
    }
       
    correct_size(reg, &w, &h);
    
    fp->g.w=w;
    fp->g.h=h;
    
    if(!(rq_flags&REGION_RQGEOM_WEAK_X) 
       && rq_flags&REGION_RQGEOM_WEAK_W){
        fp->g.x=x_;
    }else if(rq_flags&REGION_RQGEOM_WEAK_X){
        switch((*szplcy)&SIZEPOLICY_HORIZ_MASK){
        case SIZEPOLICY_HORIZ_CENTER:
            fp->g.x=max_geom.x+(max_geom.w-w)/2;
            break;
 
        case SIZEPOLICY_HORIZ_LEFT:
            fp->g.x=max_geom.x;
            break;
            
        case SIZEPOLICY_HORIZ_RIGHT:
            fp->g.x=max_geom.x+max_geom.w-w;
            break;
            
        default:
            fp->g.x=fit_x(rq_geom->x, w, &max_geom);
            break;
        }
    }else{
        fp->g.x=fit_x(rq_geom->x, w, &max_geom);
    }
    
    if(!(rq_flags&REGION_RQGEOM_WEAK_Y)
       && rq_flags&REGION_RQGEOM_WEAK_H){
        fp->g.y=y_;
    }else if(rq_flags&REGION_RQGEOM_WEAK_Y){
        switch((*szplcy)&SIZEPOLICY_VERT_MASK){
        case SIZEPOLICY_VERT_CENTER:
            fp->g.y=max_geom.y+(max_geom.h-h)/2;
            break;
            
        case SIZEPOLICY_VERT_TOP:
            fp->g.y=max_geom.y;
            break;
            
        case SIZEPOLICY_VERT_BOTTOM:
            fp->g.y=max_geom.y+max_geom.h-h;
            break;
            
        default:
            fp->g.y=fit_y(rq_geom->y, h, &max_geom);
            break;
        }
    }else{
        fp->g.y=fit_y(rq_geom->y, h, &max_geom);
    }
    
    fp->mode=REGION_FIT_EXACT;
    
    (*szplcy)&=~(SIZEPOLICY_VERT_MASK|SIZEPOLICY_HORIZ_MASK);
    
    *szplcy|=( (fullw || fp->g.x<=max_geom.x ? SIZEPOLICY_HORIZ_LEFT : 0)
              |(fullw || fp->g.x+fp->g.w>=max_geom.x+max_geom.w ? SIZEPOLICY_HORIZ_RIGHT : 0)
              |(fullh || fp->g.y<=max_geom.y ? SIZEPOLICY_VERT_TOP : 0)
              |(fullh || fp->g.y+fp->g.h>=max_geom.y+max_geom.h ? SIZEPOLICY_VERT_BOTTOM : 0));
}


void sizepolicy(WSizePolicy *szplcy, WRegion *reg,
                const WRectangle *rq_geom, int rq_flags,
                WFitParams *fp)
{
    /* TODO: use WEAK_* in rq_flags */
    
    WRectangle tmp;
    if(rq_geom!=NULL)
        tmp=*rq_geom;
    else if(reg!=NULL)
        tmp=REGION_GEOM(reg);
    else
        tmp=fp->g;

    switch((*szplcy)&SIZEPOLICY_MASK){
    case SIZEPOLICY_GRAVITY:
        gravity_stretch_policy(*szplcy, reg, &tmp, fp, FALSE, FALSE);
        break;
        
    case SIZEPOLICY_STRETCH_LEFT:
        gravity_stretch_policy(SIZEPOLICY_HORIZ_LEFT|SIZEPOLICY_VERT_CENTER, 
                               reg, &tmp, fp, FALSE, TRUE);
        break;
        
    case SIZEPOLICY_STRETCH_RIGHT:
        gravity_stretch_policy(SIZEPOLICY_HORIZ_RIGHT|SIZEPOLICY_VERT_CENTER, 
                               reg, &tmp, fp, FALSE, TRUE);
        break;
        
    case SIZEPOLICY_STRETCH_TOP:
        gravity_stretch_policy(SIZEPOLICY_VERT_TOP|SIZEPOLICY_HORIZ_CENTER, 
                               reg, &tmp, fp, TRUE, FALSE);
        break;
        
    case SIZEPOLICY_STRETCH_BOTTOM:
        gravity_stretch_policy(SIZEPOLICY_VERT_BOTTOM|SIZEPOLICY_HORIZ_CENTER, 
                               reg, &tmp, fp, TRUE, FALSE);
        break;
        
    case SIZEPOLICY_FULL_EXACT:
        gravity_stretch_policy(SIZEPOLICY_VERT_CENTER|SIZEPOLICY_HORIZ_CENTER, 
                               reg, &tmp, fp, TRUE, TRUE);
        break;
        
    case SIZEPOLICY_FREE:
        rectangle_constrain(&tmp, &(fp->g));
        correct_size(reg, &tmp.w, &tmp.h);
        fp->g=tmp;
        fp->mode=REGION_FIT_EXACT;
        break;

    case SIZEPOLICY_FREE_GLUE:
        sizepolicy_free_snap(szplcy, reg, &tmp, rq_flags, fp);
        break;
        
    case SIZEPOLICY_FULL_BOUNDS:
    default:
        fp->mode=REGION_FIT_BOUNDS;
        break;
    }
}


struct szplcy_spec {
    const char *spec;
    int szplcy;
};


/* translation table for sizepolicy specifications */
static struct szplcy_spec szplcy_specs[] = {
    {"default",         SIZEPOLICY_DEFAULT},
    {"full",            SIZEPOLICY_FULL_EXACT},
    {"full_bounds",     SIZEPOLICY_FULL_BOUNDS},
    {"free",            SIZEPOLICY_FREE},
    {"free_glue",       SIZEPOLICY_FREE_GLUE},
    {"northwest",       SIZEPOLICY_GRAVITY_NORTHWEST},
    {"north",           SIZEPOLICY_GRAVITY_NORTH},
    {"northeast",       SIZEPOLICY_GRAVITY_NORTHEAST},
    {"west",            SIZEPOLICY_GRAVITY_WEST},
    {"center",          SIZEPOLICY_GRAVITY_CENTER},
    {"east",            SIZEPOLICY_GRAVITY_EAST},
    {"southwest",       SIZEPOLICY_GRAVITY_SOUTHWEST},
    {"south",           SIZEPOLICY_GRAVITY_SOUTH},
    {"southeast",       SIZEPOLICY_GRAVITY_SOUTHEAST},
    {"stretch_top",     SIZEPOLICY_STRETCH_TOP},
    {"stretch_bottom",  SIZEPOLICY_STRETCH_BOTTOM},
    {"stretch_left",    SIZEPOLICY_STRETCH_LEFT},
    {"stretch_right",   SIZEPOLICY_STRETCH_RIGHT},
    {"free_glue_northwest",  SIZEPOLICY_FREE_GLUE__NORTHWEST},
    {"free_glue_north",      SIZEPOLICY_FREE_GLUE__NORTH},
    {"free_glue_northeast",  SIZEPOLICY_FREE_GLUE__NORTHEAST},
    {"free_glue_west",       SIZEPOLICY_FREE_GLUE__WEST},
    {"free_glue_center",     SIZEPOLICY_FREE_GLUE__CENTER},
    {"free_glue_east",       SIZEPOLICY_FREE_GLUE__EAST},
    {"free_glue_southwest",  SIZEPOLICY_FREE_GLUE__SOUTHWEST},
    {"free_glue_south",      SIZEPOLICY_FREE_GLUE__SOUTH},
    {"free_glue_southeast",  SIZEPOLICY_FREE_GLUE__SOUTHEAST},
    { NULL,             SIZEPOLICY_DEFAULT}   /* end marker */
};


bool string2sizepolicy(const char *szplcy, WSizePolicy *value)
{
    const struct szplcy_spec *sp;
    
    *value=SIZEPOLICY_DEFAULT;

    for(sp=szplcy_specs; sp->spec; ++sp){
	if(strcasecmp(szplcy,sp->spec)==0){
	    *value=sp->szplcy;
	    return TRUE;
        }
    }
    
    return FALSE;
}
