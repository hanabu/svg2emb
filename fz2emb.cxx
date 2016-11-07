/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Copyright (c) 2016, Hanabusa Masahiro All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * 3.  The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISE OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ( BSD license without advertising clause )
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/*
 * run fz2emb as:
 *
 *  funzip input.fzz *.fz | fz2emb output.pes
 */


#include<cmath>
#include<cstring>
#include<cstdio>
#include<memory>
#include<vector>
#include<stdexcept>

/* libxml2 */
#include<libxml/parser.h>
#include<libxml/xpath.h>

#include<cubicbezier.hxx>


static
std::vector<std::vector<math::vector2d> > parse_fz() throw(std::runtime_error)
{
  std::vector<std::vector<math::vector2d> > stitches;

  xmlDocPtr	xmldoc = xmlReadFd(fileno(stdin), NULL, NULL, XML_PARSE_RECOVER);
  if( NULL==xmldoc ){
    throw std::runtime_error("XML parse error.");
  }


  /* find module/instances/instance elements */
  xmlXPathContextPtr xpathctx = xmlXPathNewContext(xmldoc);
  xmlXPathObjectPtr xpathinsts
    = xmlXPathEvalExpression((const xmlChar*)"/module/instances/instance",
                             xpathctx);
  if( NULL!=xpathinsts && NULL!=xpathinsts->nodesetval ){
    for(int i=0; i<xpathinsts->nodesetval->nodeNr; ++i){
      xmlNodePtr xmlinst = xpathinsts->nodesetval->nodeTab[i];

      /* get moduleIdRef attribute */
      xmlChar* modref = xmlGetProp(xmlinst, (const xmlChar*)"moduleIdRef");
      bool iswire = ( 0==xmlStrcmp(modref,(const xmlChar*)"WireModuleID") );
      xmlFree(modref);

      if( ! iswire ){
        continue;
      }

      /* parse wire */
      /* find views/pcbView */
      xmlXPathObjectPtr xpathviews
        = xmlXPathNodeEval(xmlinst, (const xmlChar*)"views/pcbView", xpathctx);
      if( NULL!=xpathviews && NULL!=xpathviews->nodesetval &&
          0<xpathviews->nodesetval->nodeNr){
        xmlNodePtr xmlview = xpathviews->nodesetval->nodeTab[0];

        /* find geometry */
        xmlXPathObjectPtr xpathgeoms
          = xmlXPathNodeEval(xmlview, (const xmlChar*)"geometry", xpathctx);
        if( NULL!=xpathgeoms && NULL!=xpathgeoms->nodesetval &&
            0<xpathgeoms->nodesetval->nodeNr){
          xmlNodePtr xmlgeom = xpathgeoms->nodesetval->nodeTab[0];

          /* */
          xmlElemDump(stdout, xmldoc, xmlgeom);
          xmlChar* attr;

          attr = xmlGetProp(xmlgeom, (const xmlChar*)"x");
          double x = strtod((const char*) attr, NULL);
          xmlFree(attr);

          attr = xmlGetProp(xmlgeom, (const xmlChar*)"y");
          double y = strtod((const char*) attr, NULL);
          xmlFree(attr);

          attr = xmlGetProp(xmlgeom, (const xmlChar*)"x1");
          double x1 = strtod((const char*) attr, NULL);
          xmlFree(attr);

          attr = xmlGetProp(xmlgeom, (const xmlChar*)"y1");
          double y1 = strtod((const char*) attr, NULL);
          xmlFree(attr);

          attr = xmlGetProp(xmlgeom, (const xmlChar*)"x2");
          double x2 = strtod((const char*) attr, NULL);
          xmlFree(attr);

          attr = xmlGetProp(xmlgeom, (const xmlChar*)"y2");
          double y2 = strtod((const char*) attr, NULL);
          xmlFree(attr);

          printf("\n(%f,%f)-(%f,%f)\n", x+x1, y+y1, x+x2, y+y2);
        }
        xmlXPathFreeObject(xpathgeoms);
      }

      xmlXPathFreeObject(xpathviews);
    }
  }

  xmlXPathFreeObject(xpathinsts);
  xmlXPathFreeContext(xpathctx);
  xmlFreeDoc(xmldoc);
 
  return stitches;
}




/*
 * main 
 */
int main(int argc, char* argv[])
{


  try{

    std::vector<std::vector<math::vector2d> > stitches
      = parse_fz();
    if( stitches.empty() ){
      fputs("Empty Fritzing PCB.\n", stdout);
      return 1;
    }

    /* ToDo : reorder stitches */

    //    save_embroidery(outfile, stitches);

    return 0;
  }catch(std::exception e){
    fputs(e.what(), stdout);
    fputs("\n", stdout);
    return 1;
  }
}
