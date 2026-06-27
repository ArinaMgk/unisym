	;: union
	union_name UNION
	union_fields
	union_name ENDS
	;: a nested case
	struct_name STRUCT
	structure_fields
	UNION union_name
	union_fields
	ENDS
	struct_name ENDS
