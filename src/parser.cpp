
#include <vec3.hpp>
#include <cstdlib>
#include <cstring>

glm::vec3 atovec3(const char *str)
{
	glm::vec3 out(0);
	bool b=false;
	if(str[0]=='{'){
		b = true;
		str++;
	}
	if(str[0]=='[')
		str++;
	if(str[0]==' ')
		str++;
	out.x = atof(str);
	str = strstr(str, " ")+1;
	out.y = atof(str);
	str = strstr(str, " ")+1;
	out.z = atof(str);
	if(b)
		out /= 255.0f;
	return out;
}

char *ParseString(char *str, char *token, bool line)
{
	int c, len;
	
	if(!token)
		return 0;
	
	len=0;
	token[0] = 0;
	
	if(!str)
		return 0;
	
skipwhite:
	while(( c = ((unsigned char)*str)) <= ' ' )
	{
		if( c == 0 )
			return 0;	// end of file;
		str++;
	}

	// skip // comments
	if( c=='/' && str[1] == '/' )
	{
		while( *str && *str != '\n' )
			str++;
		goto skipwhite;
	}
	
	// handle quoted strings specially
	if( c == '\"' )
	{
		str++;
		while( 1 )
		{
			c = (unsigned char)*str;

			// unexpected line end
			if( !c )
			{
				token[len] = 0;
				return str;
			}
			str++;

			if( c == '\\' && *str == '"' )
			{
				token[len++] = *str++;
				continue;
			}

			if( c == '\"' )
			{
				token[len] = 0;
				return str;
			}
			token[len] = c;
			len++;
		}
	}
	
	// parse single characters
	if( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' )
	{
		token[len] = c;
		len++;
		token[len] = 0;
		return str + 1;
	}
	
	// parse a regular word
	do
	{
		token[len] = c;
		str++;
		len++;
		c = ((unsigned char)*str);

		if( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' )
			break;
	} while( c > 32 || (line&&c==' '));
	
	token[len] = 0;

	return str;
}
