#ifndef GPATTERN_H
#define GPATTERN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GPATTERN_C
extern int epnum[MAX_CHN];
extern int eppos;
extern int epview;
extern int epcolumn;
extern int epchn;
extern int epoctave;
extern int epmarkchn;
extern int epmarkstart;
extern int epmarkend;
#endif

void patterncommands(void);
void nextpattern(void);
void prevpattern(void);
void patternup(void);
void patterndown(void);
void shrinkpattern(void);
void expandpattern(void);
void splitpattern(void);
void joinpattern(void);

#ifdef __cplusplus
}
#endif

#endif
