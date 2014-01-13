/**
 * header file to parse the command line options
 */
#ifndef __OPTIONS_H__
#define __OPTIONS_H__

/**
 * Structure for the options
 */
typedef struct {
 int  poolfee;
 char *poolip;
 int  poolport;
 char *pooluser;
 char *poolpassword;
} CmdOpts;

/**
 * read the comand line options into a CmdOpts struct
 */
CmdOpts *get_opts(int argc, char *argv[]);

#endif /* __OPTIONS_H__ */
