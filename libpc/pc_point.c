/***********************************************************************
* pc_point.c
*
*  Pointclound point handling. Create, get and set values from the
*  basic PCPOINT structure.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"
#include "stringbuffer.h"

PCPOINT * 
pc_point_make(const PCSCHEMA *s)
{
	size_t sz;
	PCPOINT *pt;
	
	if ( ! s )
	{
		pcerror("null schema passed into pc_point_make");
		return NULL;
	}
	
	/* Width of the data area */
	sz = s->size;
	if ( ! sz )
	{
		pcerror("invalid size calculation in pc_point_make");
		return NULL;
	}
	
	/* Make our own data area */
	pt = pcalloc(sizeof(PCPOINT)); 
	pt->data = pcalloc(sz);
	
	/* Set up basic info */
	pt->schema = s;
	pt->readonly = PC_FALSE;
	return pt;
};

PCPOINT * 
pc_point_from_data(const PCSCHEMA *s, const uint8_t *data)
{
	size_t sz;
	PCPOINT *pt;
	uint32_t pcid;
	
	if ( ! s )
	{
		pcerror("null schema passed into pc_point_from_data");
		return NULL;
	}
	
	/* Reference the external data */
	pt = pcalloc(sizeof(PCPOINT));
	pt->data = (uint8_t*)data;
	
	/* Set up basic info */
	pt->schema = s;
	pt->readonly = PC_TRUE;
	return pt;
}

PCPOINT * 
pc_point_from_data_rw(const PCSCHEMA *s, uint8_t *data)
{
	PCPOINT *pt = pc_point_from_data(s, data);
	pt->readonly = PC_FALSE;
	return pt;
}


void
pc_point_free(PCPOINT *pt)
{
	if ( ! pt->readonly )
	{
		pcfree(pt->data);
	}
	pcfree(pt);
}

static double
pc_point_get_double(const PCPOINT *pt, const PCDIMENSION *d)
{
	uint8_t *ptr;
	double val;
	
	/* Read raw value from byte buffer */
	ptr = pt->data + d->byteoffset;
	val = pc_double_from_ptr(ptr, d->interpretation);

	/* Scale value */
	if ( d->scale )
		val *= d->scale;

	/* Offset value */
	if ( d->offset )
		val += d->offset;
	
	return val;
}

double
pc_point_get_double_by_name(const PCPOINT *pt, const char *name)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension_by_name(pt->schema, name);
	return pc_point_get_double(pt, d);
}

double
pc_point_get_double_by_index(const PCPOINT *pt, uint32_t idx)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension(pt->schema, idx);
	return pc_point_get_double(pt, d);
}

int 
pc_point_set_double(PCPOINT *pt, const PCDIMENSION *d, double val)
{
	uint8_t *ptr;

	/* Offset value */
	if ( d->offset )
		val -= d->offset;

	/* Scale value */
	if ( d->scale )
		val /= d->scale;

	/* Get pointer into byte buffer */
	ptr = pt->data + d->byteoffset;
	
	return pc_double_to_ptr(ptr, d->interpretation, val);
}

int 
pc_point_set_double_by_index(PCPOINT *pt, uint32_t idx, double val)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension(pt->schema, idx);
	return pc_point_set_double(pt, d, val);
}

int 
pc_point_set_double_by_name(PCPOINT *pt, const char *name, double val)
{
	PCDIMENSION *d;
	d = pc_schema_get_dimension_by_name(pt->schema, name);
	return pc_point_set_double(pt, d, val);
}

double
pc_point_get_x(const PCPOINT *pt)
{
	return pc_point_get_double_by_index(pt, pt->schema->x_position);
}

double 
pc_point_get_y(const PCPOINT *pt)
{
	return pc_point_get_double_by_index(pt, pt->schema->y_position);
}

char *
pc_text_from_point(const PCPOINT *pt, size_t *sersize)
{
	/* ( <pcid> : <dim1>, <dim2>, <dim3>, <dim4> )*/
	stringbuffer_t *sb = stringbuffer_create();
	char *str;
	int i;
	
	stringbuffer_aprintf(sb, "( %d : ", pt->schema->pcid);
	for ( i = 0; i < pt->schema->ndims; i++ )
	{
		if ( i )
		{
			stringbuffer_append(sb, ", ");
		}
		stringbuffer_aprintf(sb, "%g", pc_point_get_double_by_index(pt, i));
	}
	str = stringbuffer_getstringcopy(sb);
	stringbuffer_destroy(sb);
	return str;
}

PCPOINT * 
pc_point_from_double_array(const PCSCHEMA *s, double *array, uint32_t nelems)
{
	int i;
	PCPOINT *pt;

	if ( ! s )
	{
		pcerror("null schema passed into pc_point_from_double_array");
		return NULL;
	}
	
	if ( s->ndims != nelems )
	{
		pcerror("number of elements in schema and array differ in pc_point_from_double_array");
		return NULL;
	}

	/* Reference the external data */
	pt = pcalloc(sizeof(PCPOINT)); 
	pt->data = pcalloc(s->size);
	pt->schema = s;
	pt->readonly = PC_FALSE;
	
	for ( i = 0; i < nelems; i++ )
	{
		if ( PC_FAILURE == pc_point_set_double_by_index(pt, i, array[i]) )
		{
			pcerror("failed to write value into dimension %d in pc_point_from_double_array", i);
			return NULL;
		}
	}
	
	return pt;
}	

	
