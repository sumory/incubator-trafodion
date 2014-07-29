/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExpSeqGen.cpp
 * Description:  
 *               
 *               
 * Created:      7/20/2014
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "SQLCLIdev.h"
#include "ExpSeqGen.h"

//**************************************************************************
// class SeqGenEntry
//**************************************************************************
SeqGenEntry::SeqGenEntry(SequenceGeneratorAttributes &sga, CollHeap * heap)
  : sga_(sga),
    heap_(heap)
{
  fetchNewRange_ = TRUE;
  cliInterfaceArr_ = NULL;
}

short SeqGenEntry::fetchNewRange()
{
  Lng32 cliRC = 0;

  // fetch new range from Seq Generator database
  SequenceGeneratorAttributes sga;
  sga = sga_;
  if (sga.getSGCache() == 0)
    sga.setSGCache(1); 
  cliRC = SQL_EXEC_SeqGenCliInterface(&cliInterfaceArr_, &sga);
  if (cliRC < 0)
    return (short)cliRC;

  cachedStartValue_ = sga.getSGNextValue();
  cachedCurrValue_ = cachedStartValue_;
  cachedEndValue_ = sga.getSGEndValue();

  if (cachedStartValue_ > sga.getSGMaxValue())
    {
      return -1579; // max reached
    }

  fetchNewRange_ = FALSE;

  return 0;
}

short SeqGenEntry::getNextSeqVal(Int64 &seqVal)
{
  short rc = 0;

  if (NOT fetchNewRange_)
    {
      cachedCurrValue_ += sga_.getSGIncrement();
      if (cachedCurrValue_ > cachedEndValue_)
	fetchNewRange_ = TRUE;
    }

  if (fetchNewRange_)
    {
      rc = fetchNewRange();
      if (rc)
	return rc;
    }

  seqVal = cachedCurrValue_;

  return 0;
}

short SeqGenEntry::getCurrSeqVal(Int64 &seqVal)
{
  short rc = 0;

  if (fetchNewRange_)
    {
      rc = fetchNewRange();
      if (rc)
	return rc;
    }

  seqVal = cachedCurrValue_;
  
  return 0;
}

SequenceValueGenerator::SequenceValueGenerator(CollHeap * heap)
  : heap_(heap)
{
  sgQueue_ = new(heap_) HashQueue(heap);
}

SeqGenEntry * SequenceValueGenerator::getEntry(SequenceGeneratorAttributes &sga)
{
  Int64 hashVal = sga.getSGObjectUID().get_value();

  sgQueue()->position((char*)&hashVal, sizeof(hashVal));

  SeqGenEntry * sge = NULL;
  while ((sge = (SeqGenEntry *)sgQueue()->getNext()) != NULL)
    {
      if (sge->seqGenAttrs().getSGObjectUID().get_value() == hashVal)
	break;
    }

  if (! sge)
    {
      sge = new(getHeap()) SeqGenEntry(sga, getHeap());
      sgQueue()->insert((char*)&hashVal, sizeof(hashVal), sge);
    }

  return sge;
}

short SequenceValueGenerator::getNextSeqVal(SequenceGeneratorAttributes &sga,
					    Int64 &seqVal)
{

  SeqGenEntry * sge = getEntry(sga);
  return sge->getNextSeqVal(seqVal);
}

short SequenceValueGenerator::getCurrSeqVal(SequenceGeneratorAttributes &sga,
					    Int64 &seqVal)
{

  SeqGenEntry * sge = getEntry(sga);
  return sge->getCurrSeqVal(seqVal);
}

