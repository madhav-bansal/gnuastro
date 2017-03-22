/*********************************************************************
Statistics - Statistical analysis on input dataset.
Statistics is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2015, Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <math.h>
#include <errno.h>
#include <error.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/fits.h>
#include <gnuastro/tile.h>
#include <gnuastro/blank.h>
#include <gnuastro/multidim.h>
#include <gnuastro/arithmetic.h>
#include <gnuastro/statistics.h>
#include <gnuastro/interpolate.h>
#include <gnuastro/permutation.h>

#include <timing.h>
#include <checkset.h>

#include "main.h"

#include "ui.h"
#include "statistics.h"





/*******************************************************************/
/**************           Print in one row           ***************/
/*******************************************************************/
static gal_data_t *
statistics_pull_out_element(gal_data_t *input, size_t index)
{
  size_t dsize=1;
  gal_data_t *out=gal_data_alloc(NULL, input->type, 1, &dsize,
                                 NULL, 1, -1, NULL, NULL, NULL);
  gal_data_copy_element_same_type(input, index, out->array);
  return out;
}





static double
statistics_read_check_args(struct statisticsparams *p)
{
  double d;
  if(p->tp_args==NULL)
    error(EXIT_FAILURE, 0, "a bug! Not enough arguments for the "
          "requested single measurement options. Please contact "
          "us at %s so we can address the problem",
          PACKAGE_BUGREPORT);
  gal_linkedlist_pop_from_dll(&p->tp_args, &d);
  return d;
}





static void
statistics_print_one_row(struct statisticsparams *p)
{
  int mustfree;
  char *toprint;
  size_t dsize=1;
  double arg, *d;
  struct gal_linkedlist_ill *tmp;
  gal_data_t *tmpv, *out=NULL, *num=NULL, *min=NULL, *max=NULL;
  gal_data_t *sum=NULL, *med=NULL, *meanstd=NULL, *modearr=NULL;


  /* The user can ask for any of the operators more than once, also some
     operators might return more than one usable value (like mode). So we
     will calculate the desired values once, and then print them. */
  for(tmp=p->singlevalue; tmp!=NULL; tmp=tmp->next)
    switch(tmp->v)
      {
      /* Calculate respective values. Checking with `if(num==NULL)' gives
         compiler warnings of `this if clause does not guard ...'. So we
         are using this empty-if and else statement. */
      case ARGS_OPTION_KEY_NUMBER:
        num = num ? num : gal_statistics_number(p->input);           break;
      case ARGS_OPTION_KEY_MINIMUM:
        min = min ? min : gal_statistics_minimum(p->input);          break;
      case ARGS_OPTION_KEY_MAXIMUM:
        max = max ? max : gal_statistics_maximum(p->input);          break;
      case ARGS_OPTION_KEY_SUM:
        sum = sum ? sum : gal_statistics_sum(p->input);              break;
      case ARGS_OPTION_KEY_MEDIAN:
        med = med ? med : gal_statistics_median(p->sorted, 0); break;
      case ARGS_OPTION_KEY_MEAN:
      case ARGS_OPTION_KEY_STD:
        meanstd = meanstd ? meanstd : gal_statistics_mean_std(p->input);
        break;
      case ARGS_OPTION_KEY_MODE:
      case ARGS_OPTION_KEY_MODEQUANT:
      case ARGS_OPTION_KEY_MODESYM:
      case ARGS_OPTION_KEY_MODESYMVALUE:
        modearr = ( modearr ? modearr
                    : gal_statistics_mode(p->sorted, p->mirrordist, 0) );
        d=modearr->array;
        if(d[2]<GAL_STATISTICS_MODE_GOOD_SYM) d[0]=d[1]=NAN;
        break;

      /* Will be calculated as printed. */
      case ARGS_OPTION_KEY_QUANTILE:
      case ARGS_OPTION_KEY_QUANTFUNC:
        break;

      /* The option isn't recognized. */
      default:
        error(EXIT_FAILURE, 0, "A bug! Operation code %d not recognized in "
              "`statistics_print_one_row'. Please contact us at %s so we "
              "can address the problem", tmp->v, PACKAGE_BUGREPORT);
      }


  /* Print every requested number. */
  for(tmp=p->singlevalue; tmp!=NULL; tmp=tmp->next)
    {
      /* By default don't free anything. */
      mustfree=0;

      /* Get the output. */
      switch(tmp->v)
        {
        /* Previously calculated values. */
        case ARGS_OPTION_KEY_NUMBER:     out=num;                  break;
        case ARGS_OPTION_KEY_MINIMUM:    out=min;                  break;
        case ARGS_OPTION_KEY_MAXIMUM:    out=max;                  break;
        case ARGS_OPTION_KEY_SUM:        out=sum;                  break;
        case ARGS_OPTION_KEY_MEDIAN:     out=med;                  break;
        case ARGS_OPTION_KEY_MEAN:
          out=statistics_pull_out_element(meanstd, 0); mustfree=1; break;
        case ARGS_OPTION_KEY_STD:
          out=statistics_pull_out_element(meanstd, 1); mustfree=1; break;
        case ARGS_OPTION_KEY_MODE:
          out=statistics_pull_out_element(modearr, 0); mustfree=1; break;
        case ARGS_OPTION_KEY_MODEQUANT:
          out=statistics_pull_out_element(modearr, 1); mustfree=1; break;
        case ARGS_OPTION_KEY_MODESYM:
          out=statistics_pull_out_element(modearr, 2); mustfree=1; break;
        case ARGS_OPTION_KEY_MODESYMVALUE:
          out=statistics_pull_out_element(modearr, 3); mustfree=1; break;

        /* Not previously calculated. */
        case ARGS_OPTION_KEY_QUANTILE:
          arg = statistics_read_check_args(p);
          out = gal_statistics_quantile(p->sorted, arg, 0);
          break;

        case ARGS_OPTION_KEY_QUANTFUNC:
          arg = statistics_read_check_args(p);
          tmpv = gal_data_alloc(NULL, GAL_DATA_TYPE_FLOAT64, 1, &dsize,
                                NULL, 1, -1, NULL, NULL, NULL);
          *((double *)(tmpv->array)) = arg;
          tmpv = gal_data_copy_to_new_type_free(tmpv, p->input->type);
          out = gal_statistics_quantile_function(p->sorted, tmpv, 0);
          break;
        }

      /* Print the number. */
      toprint=gal_data_write_to_string(out->array, out->type, 0);
      printf("%s ", toprint);
      free(toprint);

      /* Clean up (if necessary). */
      if(mustfree) gal_data_free(out);
    }


  /* Print a new line. */
  printf("\n");


  /* Clean any of the allocated arrays. */
  if(num)     gal_data_free(num);
  if(min)     gal_data_free(min);
  if(max)     gal_data_free(max);
  if(sum)     gal_data_free(sum);
  if(med)     gal_data_free(med);
  if(meanstd) gal_data_free(meanstd);
  if(modearr) gal_data_free(modearr);
}




















/*******************************************************************/
/**************         Single value on tile         ***************/
/*******************************************************************/
static void
statistics_interpolate_and_write(struct statisticsparams *p,
                                 gal_data_t *values)
{
  gal_data_t *interpd;
  char *output=p->cp.output;
  struct gal_tile_two_layer_params *tl=&p->cp.tl;


  /* Do the interpolation (if necessary). */
  if( p->interpolate
      && !(p->interponlyblank && gal_blank_present(values)==0) )
    {
      interpd=gal_interpolate_close_neighbors(values, tl, p->interpnumngb,
                                              p->cp.numthreads,
                                              p->interponlyblank);
      gal_data_free(values);
      values=interpd;
    }

  /* Write the values. */
  gal_tile_full_values_write(values, tl, output, PROGRAM_STRING);

  /* Clean up. */
  gal_data_free(values);
}





static void
statistics_on_tile(struct statisticsparams *p)
{
  double arg;
  uint8_t type;
  size_t tind, dsize=1, mind;
  gal_data_t *tmp, *tmpv, *ttmp;
  gal_data_t *check, *tile, *values;
  struct gal_linkedlist_ill *operation;
  struct gal_options_common_params *cp=&p->cp;
  struct gal_tile_two_layer_params *tl=&p->cp.tl;

  /* Set the output name. */
  if(cp->output==NULL)
    cp->output=gal_checkset_automatic_output(cp, p->inputname,
                                             "_ontile.fits");

  /* Make the tile structure. */
  gal_tile_full_two_layers(p->input, tl);


  /* Save the tile IDs if they are requested. */
  if(tl->tilecheckname)
    {
      check=gal_tile_block_check_tiles(tl->tiles);
      if(p->inputformat==INPUT_FORMAT_IMAGE)
        gal_fits_img_write(check, tl->tilecheckname, NULL, PROGRAM_NAME);
      else
        gal_table_write(check, NULL, cp->tableformat, tl->tilecheckname,
                        cp->dontdelete);
      gal_data_free(check);
    }

  /* Do the operation on each tile. */
  for(operation=p->singlevalue; operation!=NULL; operation=operation->next)
    {
      /* Set the type of the output array. */
      switch(operation->v)
        {
        case ARGS_OPTION_KEY_NUMBER:
          type=GAL_DATA_TYPE_INT32; break;

        case ARGS_OPTION_KEY_MINIMUM:
        case ARGS_OPTION_KEY_MAXIMUM:
        case ARGS_OPTION_KEY_MEDIAN:
        case ARGS_OPTION_KEY_MODE:
        case ARGS_OPTION_KEY_QUANTFUNC:
          type=p->input->type; break;

        case ARGS_OPTION_KEY_SUM:
        case ARGS_OPTION_KEY_MEAN:
        case ARGS_OPTION_KEY_STD:
        case ARGS_OPTION_KEY_QUANTILE:
        case ARGS_OPTION_KEY_MODEQUANT:
        case ARGS_OPTION_KEY_MODESYM:
        case ARGS_OPTION_KEY_MODESYMVALUE:
          type=GAL_DATA_TYPE_FLOAT64; break;

        default:
          error(EXIT_FAILURE, 0, "a bug! %d is not a recognized operation "
                "code in `statistics_on_tile'", operation->v);
        }

      /* Allocate the space necessary to keep the value for each tile. */
      values=gal_data_alloc(NULL, type, p->input->ndim, tl->numtiles, NULL,
                            0, p->input->minmapsize, NULL, NULL, NULL);

      /* Read the argument for those operations that need it. This is done
         here, because below, the functions are repeated on each tile. */
      switch(operation->v)
        {
        case ARGS_OPTION_KEY_QUANTILE:
          arg = statistics_read_check_args(p);
          break;
        case ARGS_OPTION_KEY_QUANTFUNC:
          arg = statistics_read_check_args(p);
          tmpv = gal_data_alloc(NULL, GAL_DATA_TYPE_FLOAT64, 1, &dsize,
                                NULL, 1, -1, NULL, NULL, NULL);
          *((double *)(tmpv->array)) = arg;
          tmpv = gal_data_copy_to_new_type_free(tmpv, p->input->type);
        }

      /* Do the operation on each tile. */
      tind=0;
      for(tile=tl->tiles; tile!=NULL; tile=tile->next)
        {
          /* Do the proper operation. */
          switch(operation->v)
            {
            case ARGS_OPTION_KEY_NUMBER:
              tmp=gal_statistics_number(tile);                      break;

            case ARGS_OPTION_KEY_MINIMUM:
              tmp=gal_statistics_minimum(tile);                     break;

            case ARGS_OPTION_KEY_MAXIMUM:
              tmp=gal_statistics_maximum(tile);                     break;

            case ARGS_OPTION_KEY_MEDIAN:
              tmp=gal_statistics_median(tile, 1);                   break;

            case ARGS_OPTION_KEY_QUANTFUNC:
              tmp=gal_statistics_quantile_function(tile, tmpv, 1);  break;

            case ARGS_OPTION_KEY_SUM:
              tmp=gal_statistics_sum(tile);                         break;

            case ARGS_OPTION_KEY_MEAN:
              tmp=gal_statistics_mean(tile);                        break;

            case ARGS_OPTION_KEY_STD:
              tmp=gal_statistics_std(tile);                         break;

            case ARGS_OPTION_KEY_QUANTILE:
              tmp=gal_statistics_quantile(tile, arg, 1);            break;

            case ARGS_OPTION_KEY_MODE:
            case ARGS_OPTION_KEY_MODESYM:
            case ARGS_OPTION_KEY_MODEQUANT:
            case ARGS_OPTION_KEY_MODESYMVALUE:
              switch(operation->v)
                {
                case ARGS_OPTION_KEY_MODE:         mind=0;  break;
                case ARGS_OPTION_KEY_MODESYM:      mind=2;  break;
                case ARGS_OPTION_KEY_MODEQUANT:    mind=1;  break;
                case ARGS_OPTION_KEY_MODESYMVALUE: mind=3;  break;
                }
              tmp=gal_statistics_mode(tile, p->mirrordist, 1);
              ttmp=statistics_pull_out_element(tmp, mind);
              gal_data_free(tmp);
              tmp=ttmp;
              break;

            default:
              error(EXIT_FAILURE, 0, "a bug! please contact us at %s to "
                    "fix the problem. THe operation code %d is not "
                    "recognized in `statistics_on_tile'", PACKAGE_BUGREPORT,
                    operation->v);
            }

          /* Put the output value into the `values' array and clean up. */
          tmp=gal_data_copy_to_new_type_free(tmp, type);
          memcpy(gal_data_ptr_increment(values->array, tind++,
                                        values->type),
                 tmp->array, gal_data_sizeof(type));
          gal_data_free(tmp);
        }

      /* Do the interpolation (if necessary) and write the array into the
         output. */
      statistics_interpolate_and_write(p, values);

      /* Clean up. */
      if(operation->v==ARGS_OPTION_KEY_QUANTFUNC) gal_data_free(tmpv);
    }

  /* Clean up. */
  gal_tile_full_free_contents(tl);
}



















/*******************************************************************/
/**************             ASCII plots              ***************/
/*******************************************************************/
static void
print_ascii_plot(struct statisticsparams *p, gal_data_t *plot,
                 gal_data_t *bins, int h1_c0, int printinfo)
{
  int i, j;
  size_t *s, *sf, max=0;
  double *b, v, halfbinwidth, correction;

  /* Find the maximum of the plot. */
  sf=(s=plot->array)+plot->size; do max = *s>max ? *s : max; while(++s<sf);

  /* Print the range so the user knows. */
  if(printinfo)
    {
      b=bins->array;
      halfbinwidth = (b[1]-b[0])/2;
      printf("\nASCII %s:\n", ( h1_c0 ? "Histogram" :
                                "Cumulative frequency plot") );
      if(h1_c0) printf("Number: %zu\n", p->input->size);
      printf("Y: (linear: 0 to %zu)\n", max);
      printf("X: (linear: %g -- %g, in %zu bins)\n", b[0]-halfbinwidth,
             b[ bins->size - 1 ] + halfbinwidth, bins->size);
    }

  /* Print the ASCII plot: */
  s=plot->array;
  correction = (double)(p->asciiheight) / (double)max;
  for(i=p->asciiheight;i>=0;--i)
    {
      printf(" |");
      for(j=0;j<plot->size;++j)
        {
          v = (double)s[j] * correction;
          if( v >= ((double)i-0.5f) && v > 0.0f ) printf("*");
          else printf(" ");
        }
      printf("\n");
    }
  printf(" |");
  for(j=0;j<plot->size;++j) printf("-");
  printf("\n\n");
}




static void
ascii_plots(struct statisticsparams *p)
{
  gal_data_t *bins, *hist, *cfp=NULL;

  /* Make the bins and the respective plot. */
  bins=gal_statistics_regular_bins(p->input, NULL, p->numasciibins, NAN);
  hist=gal_statistics_histogram(p->input, bins, 0, 0);
  if(p->asciicfp)
    {
      bins->next=hist;
      cfp=gal_statistics_cfp(p->input, bins, 0);
    }

  /* Print the plots. */
  if(p->asciihist)  print_ascii_plot(p, hist, bins, 1, 1);
  if(p->asciicfp)   print_ascii_plot(p, cfp,  bins, 0, 1);

  /* Clean up.*/
  gal_data_free(bins);
  gal_data_free(hist);
  if(p->asciicfp) gal_data_free(cfp);
}




















/*******************************************************************/
/*******    Histogram and cumulative frequency tables    ***********/
/*******************************************************************/
void
write_output_table(struct statisticsparams *p, gal_data_t *table,
                   char *suf, char *contents)
{
  char *output;
  int use_auto_output=0;
  char *fix, *suffix=NULL, *tmp;
  struct gal_linkedlist_stll *comments=NULL;


  /* Automatic output should be used when no output name was specified or
     we have more than one output file. */
  use_auto_output = p->cp.output ? (p->numoutfiles>1 ? 1 : 0) : 1;


  /* Set the `fix' and `suffix' strings. Note that `fix' is necessary in
     every case, even when no automatic output is to be used. Since it is
     used to determine the format of the output. */
  fix = ( p->cp.output
          ? gal_fits_name_is_fits(p->cp.output) ? "fits" : "txt"
          : "txt" );
  if(use_auto_output)
    asprintf(&suffix, "%s.%s", suf, fix);


  /* Make the output name. */
  output = ( use_auto_output
             ? gal_checkset_automatic_output(&p->cp, p->inputname, suffix)
             : p->cp.output );


  /* Write the comments, NOTE: we are writing the first two in reverse of
     the order we want them. They will later be freed as part of the list's
     freeing.*/
  tmp=gal_fits_name_save_as_string(p->inputname, p->cp.hdu);
  gal_linkedlist_add_to_stll(&comments, tmp, 0);

  asprintf(&tmp, "%s created from:", contents);
  gal_linkedlist_add_to_stll(&comments, tmp, 0);

  if(strcmp(fix, "fits"))  /* The intro info will be in FITS files anyway.*/
    gal_table_comments_add_intro(&comments, PROGRAM_STRING, &p->rawtime);


  /* Write the table. */
  gal_table_write(table, comments, p->cp.tableformat, output,
                  p->cp.dontdelete);


  /* Let the user know, if we aren't in quiet mode. */
  if(!p->cp.quiet)
    printf("%s created.\n", output);


  /* Clean up. */
  if(suffix) free(suffix);
  if(output!=p->cp.output) free(output);
  gal_linkedlist_free_stll(comments, 1);
}





static void
save_hist_and_or_cfp(struct statisticsparams *p)
{
  char *suf, *contents;
  gal_data_t *bins, *hist, *cfp=NULL;


  /* Set the bins and make the histogram, this is necessary for both the
     histogram and CFP (recall that the CFP is built from the
     histogram). */
  bins=gal_statistics_regular_bins(p->input, NULL, p->numbins,
                                   p->onebinstart);
  hist=gal_statistics_histogram(p->input, bins, p->normalize, p->maxbinone);


  /* Set the histogram as the next pointer of bins. This is again necessary
     in both cases: when only a histogram is requested, it is used for the
     plotting. When only a CFP is desired, it is used as input into
     `gal_statistics_cfp'. */
  bins->next=hist;


  /* Make the cumulative frequency plot if the user wanted it. Make the
     CFP, note that for the CFP, `maxbinone' and `normalize' are the same:
     the last bin (largest value) must be one. So if any of them are given,
     then set the last argument to 1.*/
  if(p->cumulative)
    cfp=gal_statistics_cfp(p->input, bins, p->normalize || p->maxbinone);


  /* FITS tables don't accept `uint64_t', so to be consistent, we'll conver
     the histogram and CFP to `uint32_t'.*/
  if(hist->type==GAL_DATA_TYPE_UINT64)
    hist=gal_data_copy_to_new_type_free(hist, GAL_DATA_TYPE_UINT32);
  if(cfp && cfp->type==GAL_DATA_TYPE_UINT64)
    cfp=gal_data_copy_to_new_type_free(cfp, GAL_DATA_TYPE_UINT32);


  /* Finalize the next pointers. */
  bins->next=hist;
  hist->next=cfp;


  /* Prepare the contents. */
  if(p->histogram && p->cumulative)
    { suf="_hist_cfp"; contents="Histogram and cumulative frequency plot"; }
  else if(p->histogram)
    { suf="_hist";     contents="Histogram"; }
  else
    { suf="_cfp";      contents="Cumulative frequency plot"; }


  /* Set the output file name. */
  write_output_table(p, bins, suf, contents);
}





void
print_mirror_hist_cfp(struct statisticsparams *p)
{
  size_t dsize=1;
  gal_data_t *table;
  double mirror_val;
  gal_data_t *mirror=gal_data_alloc(NULL, GAL_DATA_TYPE_FLOAT64, 1, &dsize,
                                    NULL, 1, -1, NULL, NULL, NULL);

  /* Convert the given mirror value into the type of the input dataset. */
  *((double *)(mirror->array)) = p->mirror;
  mirror=gal_data_copy_to_new_type_free(mirror, p->input->type);

  /* Make the table columns. */
  table=gal_statistics_mode_mirror_plots(p->sorted, mirror, p->numbins, 0,
                                         &mirror_val);

  if(p->mirror!=mirror_val)
    {
      fprintf(stderr, "Warning: Mirror value is %f.\n", mirror_val);
      if(!p->cp.quiet)
        fprintf(stderr, "\nNote that the mirror distribution is discrete "
                "and depends on the input data. So the closest point in "
                "the data to your desired mirror at %f was %f.\n\n",
                p->mirror, mirror_val);
    }

  /* If the mirror value was out-of-range, then no table will be made. */
  if(table)
    write_output_table(p, table, "_mirror_hist_cfp",
                       "Histogram and CFP of mirror distribution");
  else
    error(EXIT_FAILURE, 0, "mirror value %g is out of range", p->mirror);
}


















/*******************************************************************/
/**************           Basic information          ***************/
/*******************************************************************/
/* To keep things in `print_basics' clean, we'll define the input data
   here, then only print the values there. */
void
print_input_info(struct statisticsparams *p)
{
  char *str, *name, *col=NULL;

  /* Print the program name and version. */
  printf("%s\n", PROGRAM_STRING);

  /* Print the input information, if the input was a table, we also need to
     give the column information. When the column has a name, it will be
     printed, when it doesn't, we'll use the same string the user gave. */
  printf("-------\n");
  name=gal_fits_name_save_as_string(p->inputname, p->cp.hdu);
  printf("Input: %s\n", name);

  /* If a table was given, print the column. */
  if(p->column) printf("Column: %s\n",
                       p->input->name ? p->input->name : p->column);

  /* Range. */
  str=NULL;
  if( !isnan(p->greaterequal) && !isnan(p->lessthan) )
    asprintf(&str, "from (inclusive) %g, upto (exclusive) %g",
             p->greaterequal, p->lessthan);
  else if( !isnan(p->greaterequal) )
    asprintf(&str, "from (inclusive) %g", p->greaterequal);
  else if( !isnan(p->lessthan) )
    asprintf(&str, "upto (exclusive) %g", p->lessthan);
  if(str)
    {
      printf("Range: ");
      if(p->refcol)
        printf("[on column %s] ",
               p->reference->name ? p->reference->name : p->refcol);
      printf("%s.\n", str);
      free(str);
    }

  /* Units. */
  if(p->input->unit) printf("Unit: %s\n", p->input->unit);

  /* Clean up. */
  if(col) free(col);
  free(name);
  printf("-------\n");
}





/* This function will report the simple immediate statistics of the
   data. For the average and standard deviation, the unsorted data is
   used so we don't suddenly encounter rounding errors. */
void
print_basics(struct statisticsparams *p)
{
  char *str;
  int namewidth=40;
  float mirrdist=1.5;
  double mean, std, *d;
  gal_data_t *tmp, *bins, *hist;

  /* Define the input dataset. */
  print_input_info(p);

  /* Print the number: */
  printf("  %-*s %zu\n", namewidth, "Number of elements:", p->input->size);

  /* Minimum: */
  tmp=gal_statistics_minimum(p->input);
  str=gal_data_write_to_string(tmp->array, tmp->type, 0);
  printf("  %-*s %s\n", namewidth, "Minimum:", str);
  gal_data_free(tmp);
  free(str);

  /* Maximum: */
  tmp=gal_statistics_maximum(p->input);
  str=gal_data_write_to_string(tmp->array, tmp->type, 0);
  printf("  %-*s %s\n", namewidth, "Maximum:", str);
  gal_data_free(tmp);
  free(str);

  /* Find the mean and standard deviation, but don't print them, see
     explanations under median. */
  tmp=gal_statistics_mean_std(p->input);
  mean = ((double *)(tmp->array))[0];
  std  = ((double *)(tmp->array))[1];
  gal_data_free(tmp);

  /* Mode of the distribution (if it is valid). we want the mode and median
     to be found in place to save time/memory. But having a sorted array
     can decrease the floating point accuracy of the standard deviation. So
     we'll do the median calculation in the end.*/
  tmp=gal_statistics_mode(p->input, mirrdist, 1);
  d=tmp->array;
  if(d[2]>GAL_STATISTICS_MODE_GOOD_SYM)
    {        /* Same format as `gal_data_write_to_string' */
      printf("  %-*s %.10g\n", namewidth, "Mode:", d[0]);
      printf("  %-*s %.10g\n", namewidth, "Mode quantile:", d[1]);
    }
  gal_data_free(tmp);

  /* Find and print the median:  */
  tmp=gal_statistics_median(p->input, 0);
  str=gal_data_write_to_string(tmp->array, tmp->type, 0);
  printf("  %-*s %s\n", namewidth, "Median:", str);
  gal_data_free(tmp);
  free(str);

  /* Print the mean and standard deviation. Same format as
     `gal_data_write_to_string' */
  printf("  %-*s %.10g\n", namewidth, "Mean:", mean);
  printf("  %-*s %.10g\n", namewidth, "Standard deviation:", std);

  /* Ascii histogram. Note that we don't want to force the user to have the
     plotting parameters. Also, when a reference column is defined, the
     range shown in the basic information section applies to that, not the
     range of the histogram. In that case, we want to print the histogram
     information. */
  printf("-------");
  p->asciiheight = p->asciiheight ? p->asciiheight : 10;
  p->numasciibins = p->numasciibins ? p->numasciibins : 70;
  bins=gal_statistics_regular_bins(p->input, NULL, p->numasciibins, NAN);
  hist=gal_statistics_histogram(p->input, bins, 0, 0);
  if(p->refcol==NULL) printf("\nHistogram:\n");
  print_ascii_plot(p, hist, bins, 1, p->refcol ? 1 : 0);
  gal_data_free(bins);
  gal_data_free(hist);
}




















/*******************************************************************/
/**************            Sigma clipping            ***************/
/*******************************************************************/
void
print_sigma_clip(struct statisticsparams *p)
{
  float *a;
  char *mode;
  int namewidth=40;
  gal_data_t *sigclip;

  /* Set the mode for printing: */
  if( p->sigclipparam>=1.0f )
    asprintf(&mode, "for %g clips", p->sigclipparam);
  else
    asprintf(&mode, "until relative change in STD is less than %g",
             p->sigclipparam);

  /* Report the status */
  if(!p->cp.quiet)
    {
      print_input_info(p);
      printf("%g-sigma clipping steps %s:\n\n", p->sigclipmultip, mode);
    }

  /* Do the Sigma clipping: */
  sigclip=gal_statistics_sigma_clip(p->sorted, p->sigclipmultip,
                                    p->sigclipparam, p->cp.quiet);
  a=sigclip->array;

  /* Finish the introduction. */
  if(!p->cp.quiet)
    printf("-------\nSummary:\n");
  else
    printf("%g-sigma clipped %s:\n", p->sigclipmultip, mode);

  /* Print the final results: */
  printf("  %-*s %zu\n", namewidth, "Number of input elements:",
         p->input->size);
  if( p->sigclipparam < 1.0f )
    printf("  %-*s %d\n", namewidth, "Number of clips:",     sigclip->status);
  printf("  %-*s %g\n", namewidth, "Final number of elements:", a[0]);
  printf("  %-*s %g\n", namewidth, "Median:",                a[1]);
  printf("  %-*s %g\n", namewidth, "Mean:",                  a[2]);
  printf("  %-*s %g\n", namewidth, "Standard deviation:",    a[3]);

  /* Clean up. */
  free(mode);
}


















/*******************************************************************/
/**************             Main function            ***************/
/*******************************************************************/
void
statistics(struct statisticsparams *p)
{
  int print_basic_info=1;

  /* Print the one-row numbers if the user asked for them. */
  if(p->singlevalue)
    {
      print_basic_info=0;
      if(p->ontile) statistics_on_tile(p);
      else          statistics_print_one_row(p);
    }

  /* Print the ASCII plots if requested. */
  if(p->asciihist || p->asciicfp)
    {
      ascii_plots(p);
      print_basic_info=0;
    }

  /* Save the histogram and CFP as tables if requested. */
  if(p->histogram || p->cumulative)
    {
      print_basic_info=0;
      save_hist_and_or_cfp(p);
    }

  /* Print the sigma-clipped results. */
  if( !isnan(p->sigclipmultip) )
    {
      print_basic_info=0;
      print_sigma_clip(p);
    }

  /* Make the mirror table. */
  if( !isnan(p->mirror) )
    {
      print_basic_info=0;
      print_mirror_hist_cfp(p);
    }

  /* If nothing was requested print the simple statistics. */
  if(print_basic_info)
    print_basics(p);

}
