    case '"' :
        Bufsprint(&buf, "%s %s", "say", str+1);
        break;
    case '#' :
        Bufsprint(&buf, "%s %s", "poundsign", str+1);
        break;
    case '%' :
        Bufsprint(&buf, "%s %s", "percentsign", str+1);
        break;
    case '&' :
        Bufsprint(&buf, "%s %s", "ampersand", str+1);
        break;
    case '(' :
        Bufsprint(&buf, "%s %s", "lparenthesis", str+1);
        break;
    case ')' :
        Bufsprint(&buf, "%s %s", "rparenthesis", str+1);
        break;
    case '*' :
        Bufsprint(&buf, "%s %s", "asterisk", str+1);
        break;
    case '+' :
        Bufsprint(&buf, "%s %s", "plussign", str+1);
        break;
    case '-' :
        Bufsprint(&buf, "%s %s", "minussign", str+1);
        break;
    case ':' :
        Bufsprint(&buf, "%s %s", "pose", str+1);
        break;
    case ';' :
        Bufsprint(&buf, "%s %s", "semicolon", str+1);
        break;
    case '<' :
        Bufsprint(&buf, "%s %s", "backarrow", str+1);
        break;
    case '=' :
        Bufsprint(&buf, "%s %s", "equalsign", str+1);
        break;
    case '>' :
        Bufsprint(&buf, "%s %s", "forearrow", str+1);
        break;
    case '[' :
        Bufsprint(&buf, "%s %s", "lbracket", str+1);
        break;
    case '\'' :
        Bufsprint(&buf, "%s %s", "tick", str+1);
        break;
    case ']' :
        Bufsprint(&buf, "%s %s", "rbracket", str+1);
        break;
    case '`' :
        Bufsprint(&buf, "%s %s", "backtick", str+1);
        break;
    case '|' :
        Bufsprint(&buf, "%s %s", "paste", str+1);
        break;
