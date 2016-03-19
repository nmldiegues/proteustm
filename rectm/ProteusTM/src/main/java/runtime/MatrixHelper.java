package runtime;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.LinkedList;
import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 30/09/14
 */
public class MatrixHelper {
   private String outFile;
   private String kpiFile, wlFile;

   private final static String sep = ",";

   public MatrixHelper(String kpiFile, String wlFile, String outFile) {
      this.outFile = outFile;
      this.kpiFile = kpiFile;
      this.wlFile = wlFile;
   }

   /**
    * For each line in the kpiFile, find the corresponding entry in the wl file and append the content of the content of
    * the second to the first
    */
   public void merge() throws IOException, ParameterNotFoundException {
      final DoubleOutputParser kpiParser = new DoubleOutputParser(kpiFile, sep);
      final DoubleOutputParser wlParser = new DoubleOutputParser(wlFile, sep);
      final PrintWriter pw = new PrintWriter(new FileWriter(new File(outFile)));
      final String mergedHeader = mergeHeaders(kpiParser, wlParser);
      pw.println(mergedHeader);
      List<String> rows = kpiParser.rawColumn("Benchmark");
      for (String benchmark : rows) {
         String merged = mergeKpiLine(benchmark, kpiParser, wlParser);
         if (merged == null) {
            continue;
         }
         pw.println(merged);
      }
      pw.flush();
      pw.close();
   }

   /**
    * Given a file A and a file B, creates a file C that is given by all the rows of A that are also in B (and not the
    * vice versa)
    *
    * @param referenceFile
    * @param toBeCopied
    * @param out
    * @throws Exception
    */
   public static void copySameRow(String referenceFile, String toBeCopied, String out) throws Exception {
      DoubleOutputParser reference = new DoubleOutputParser(referenceFile, ",");
      DoubleOutputParser toCopy = new DoubleOutputParser(toBeCopied, ",");
      List<String> referenceIds = reference.rawColumn("Benchmark");
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      String[] header = reference.header();
      for (String s : header) {
         if (s.equals("Benchmark")) {
            pw.print(s);
         } else {
            pw.print(",");
            pw.print(s);
         }
      }
      pw.print("\n");
      for (String b : referenceIds) {
         String[] cpy = toCopy.rawRow("Benchmark", b);
         pw.print(cpy[0]);
         for (int i = 1; i < cpy.length; i++) {
            pw.print(",");
            pw.print(cpy[i]);
         }
         pw.print("\n");
      }
      pw.close();
   }

   /**
    * Merge two headers I am using this typically to concatenate the config and the workload header
    *
    * @param kpiParser
    * @param wlParser
    * @return
    */
   private String mergeHeaders(DoubleOutputParser kpiParser, DoubleOutputParser wlParser) {
      final String[] kpiHeader = kpiParser.header();
      final String[] wlHeader = wlParser.headerExcluding("Benchmark");
      return concatenate(kpiHeader, wlHeader, sep);
   }

   /**
    * Merge two lines with the given id
    *
    * @param benchmark
    * @param kpiParser
    * @param wlParser
    * @return
    */
   private String mergeKpiLine(String benchmark, DoubleOutputParser kpiParser, DoubleOutputParser wlParser) throws ParameterNotFoundException {
      String[] kpiRow = kpiParser.rawRow("Benchmark", benchmark);
      String[] wlRow = wlParser.rawRowExcludingTargetEntry("Benchmark", benchmark);

      if (wlRow == null) {
         return null;
      }
      return concatenate(kpiRow, wlRow, sep);
   }

   /**
    * Concatenate two string arrays
    *
    * @param pre
    * @param post
    * @param sep
    * @return
    */
   private static String concatenate(String[] pre, String[] post, String sep) {
      final StringBuilder sb = new StringBuilder();

      boolean first = true;
      for (String s : pre) {
         if (first) {
            first = false;
         } else {
            sb.append(sep);
         }
         sb.append(s);
      }

      for (String s : post) {
         sb.append(sep);
         sb.append(s);
      }
      return sb.toString();
   }

   /**
    * Takes the matrix in file "in" and produce "out" where each element of "in" is elevated to -1 (row and column
    * header are excluded of course from the transformation)
    *
    * @param in
    * @param out
    * @throws Exception
    */
   public static void oneOver(String in, String out) throws Exception {
      DoubleOutputParser input = new DoubleOutputParser(in, ",");
      List<String> referenceIds = input.rawColumn("Benchmark");
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      String[] header = input.header();
      for (String s : header) {
         if (s.equals("Benchmark")) {
            pw.print(s);
         } else {
            pw.print(",");
            pw.print(s);
         }
      }
      pw.print("\n");
      for (String b : referenceIds) {
         String[] cpy = input.rawRow("Benchmark", b);
         pw.print(cpy[0]);
         for (int i = 1; i < cpy.length; i++) {
            pw.print(",");
            pw.print(1.0D / (Double.parseDouble(cpy[i])));
         }
         pw.print("\n");
      }
      pw.flush();
      pw.close();
   }

   /**
    * Find the overallMax in the file inputFileMatrix
    *
    * @param inputFileMatrix
    * @param sep
    * @return
    * @throws Exception
    */
   private static double findOverallMax(String inputFileMatrix, String sep) throws Exception {
      DoubleOutputParser dop = new DoubleOutputParser(inputFileMatrix, sep);
      List<String> rawColumn = dop.rawColumn("Benchmark");
      double max = -1;
      List<String> params = dop.allParams();
      params.remove("Benchmark");
      String[] paramArray = params.toArray(new String[params.size()]);

      for (String s : rawColumn) {
         double m = dop.maxAmong("Benchmark", s, paramArray);
         if (m > max)
            max = m;
      }
      System.out.println("Max in " + inputFileMatrix + " is " + max);
      return max;
   }

   /**
    * Normalize file "in" w.r.t. fixed value "wrt", and save the new file in "out"
    *
    * @param in
    * @param out
    * @param sep
    * @param wrt
    * @throws Exception
    */
   private static void normalizeWrt(String in, String out, String sep, double wrt) throws Exception {
      DoubleOutputParser input = new DoubleOutputParser(in, sep);
      List<String> referenceIds = input.rawColumn("Benchmark");
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      String[] header = input.header();
      /*Copy header*/
      for (String s : header) {
         if (s.equals("Benchmark")) {
            pw.print(s);
         } else {
            pw.print(",");
            pw.print(s);
         }
      }
      pw.print("\n");
      /*Normalize payload*/
      for (String b : referenceIds) {
         String[] cpy = input.rawRow("Benchmark", b);
         pw.print(cpy[0]);
         for (int i = 1; i < cpy.length; i++) {
            pw.print(",");
            pw.print((Double.parseDouble(cpy[i])) / wrt);
         }
         pw.print("\n");
      }
      pw.flush();
      pw.close();
   }

   /**
    * Log10 Transform elements of the in file
    *
    * @param in
    * @param out
    * @param sep
    * @param base
    * @throws Exception
    */
   public static void logTransform(String in, String out, String sep, double base) throws Exception {
      DoubleOutputParser input = new DoubleOutputParser(in, sep);
      List<String> referenceIds = input.rawColumn("Benchmark");
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      String[] header = input.header();
         /*Copy header*/
      for (String s : header) {
         if (s.equals("Benchmark")) {
            pw.print(s);
         } else {
            pw.print(",");
            pw.print(s);
         }
      }
      pw.print("\n");
         /*Normalize payload*/
      for (String b : referenceIds) {
         String[] cpy = input.rawRow("Benchmark", b);
         pw.print(cpy[0]);
         for (int i = 1; i < cpy.length; i++) {
            pw.print(",");
            double transformed = 5 + Math.log10(Double.parseDouble(cpy[i]));
            if (transformed < 0) {
               throw new RuntimeException("less than 0");
            }
            pw.print(transformed);
         }
         pw.print("\n");
      }
      pw.flush();
      pw.close();
   }

   /**
    * Produces a file where the "in" file has been normalized w.r.t. the global max
    *
    * @param in
    * @param out
    * @param sep
    * @throws Exception
    */
   public static void normalizeWrtOverallMax(String in, String out, String sep) throws Exception {
      double max = findOverallMax(in, sep);
      normalizeWrt(in, out, sep, max);
   }

   /**
    * Produces a file where the "in" file has been  row-by-row normalized w.r.t. column "wrt"
    *
    * @param in
    * @param out
    * @param wrtIndex is 1-based
    * @param sep
    * @throws Exception
    */
   public static void normalizeWrt(String in, String out, int wrtIndex, String sep) throws Exception {
      DoubleOutputParser input = new DoubleOutputParser(in, sep);
      List<String> referenceIds = input.rawColumn("Benchmark");
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      String[] header = input.header();
      /*Copy header*/
      for (String s : header) {
         if (s.equals("Benchmark")) {
            pw.print(s);
         } else {
            pw.print(",");
            pw.print(s);
         }
      }
      pw.print("\n");
      /*Normalize payload*/
      for (String b : referenceIds) {
         String[] cpy = input.rawRow("Benchmark", b);
         //Copy benchmark name
         pw.print(cpy[0]);
         //extract normalization factor from target column
         double doubleWRT = Double.parseDouble(cpy[wrtIndex]);
         for (int i = 1; i < cpy.length; i++) {
            pw.print(",");
            pw.print((Double.parseDouble(cpy[i])) / doubleWRT);
         }
         pw.print("\n");
      }
      pw.flush();
      pw.close();
   }

   /**
    * Normalize row-wise in file w.r.t. index wrtIndex. Replace the "tobereplacedInFileName" with the name of the config
    * corresponding to wrtIndex
    *
    * @param in
    * @param wrtIndex
    * @param sep
    * @param toBeReplacedInFileName
    * @throws Exception
    */
   public static void normalizeWrt(String in, int wrtIndex, String sep, String toBeReplacedInFileName) throws Exception {
      DoubleOutputParser dop = new DoubleOutputParser(in, sep);
      String ref = dop.columnId(wrtIndex);
      String out = in.replace(toBeReplacedInFileName, ref);
      normalizeWrt(in, out, wrtIndex, sep);
   }


   public static void normalizeWrtHighestPerRow() {

   }

   /**
    * Remove all columns in file in that are not in file from. Save in file out
    *
    * @param in
    * @param from
    * @param out
    */
   public static void keepColumnFrom(String in, String from, String out, String sep) throws Exception {
      DoubleOutputParser fromDop = new DoubleOutputParser(from, sep);
      DoubleOutputParser inDop = new DoubleOutputParser(in, sep);
      String[] fromHeader = fromDop.header();
      LinkedList<Integer> list = new LinkedList<>();
      for (String s : fromHeader) {
         list.addLast(inDop.idFor(s)); //See what index is the column in the ORIGINAL file and save it
      }
      inDop.dumpToFileOnlyColumns(out, list);
   }

   /**
    * Normalize in file row-wise w.r.t. the minimum value per row
    *
    * @param in
    * @param out
    * @throws Exception
    */
   public static void normalizeWrtLowestPerRow(String in, String out) throws Exception {
      DoubleOutputParser inputParser = new DoubleOutputParser(in, sep);
      List<String> referenceIds = inputParser.rawColumn("Benchmark");
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      String[] header = inputParser.header();
      /*Copy header*/
      for (String s : header) {
         if (s.equals("Benchmark")) {
            pw.print(s);
         } else {
            pw.print(",");
            pw.print(s);
         }
      }
      pw.print("\n");
      /*Normalize payload*/
      for (String currentBenchmark : referenceIds) {
         String[] cpy = inputParser.rawRow("Benchmark", currentBenchmark);
         //Copy benchmark name
         pw.print(cpy[0]);
         //extract normalization factor from target column
         double doubleWRT = inputParser.minAmong("Benchmark", currentBenchmark, inputParser.headerExcluding("Benchmark"));
         for (int i = 1; i < cpy.length; i++) {
            pw.print(",");
            pw.print((doubleWRT / Double.parseDouble(cpy[i])));
         }
         pw.print("\n");
      }
      pw.flush();
      pw.close();
   }


   public static void normalizeWrtHighestPerRow(String in, String out) throws Exception {
        DoubleOutputParser inputParser = new DoubleOutputParser(in, sep);
        List<String> referenceIds = inputParser.rawColumn("Benchmark");
        PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
        String[] header = inputParser.header();
        /*Copy header*/
        for (String s : header) {
           if (s.equals("Benchmark")) {
              pw.print(s);
           } else {
              pw.print(",");
              pw.print(s);
           }
        }
        pw.print("\n");
        /*Normalize payload*/
        for (String currentBenchmark : referenceIds) {
           String[] cpy = inputParser.rawRow("Benchmark", currentBenchmark);
           //Copy benchmark name
           pw.print(cpy[0]);
           //extract normalization factor from target column
           double doubleWRT = inputParser.maxAmong("Benchmark", currentBenchmark, inputParser.headerExcluding("Benchmark"));
           for (int i = 1; i < cpy.length; i++) {
              pw.print(",");
              pw.print((Double.parseDouble(cpy[i]) / doubleWRT));
           }
           pw.print("\n");
        }
        pw.flush();
        pw.close();
     }


   /* MAIN instances */


   public static void Main(String[] args) throws Exception {
      final String in = "UtilityMatrix/data/141004/reduced/141003_oneOverNone_nowl_large_EDP.csv";
      final String out = "UtilityMatrix/data/141004/reduced/141003_maxMaxNorm_nowl_large_EDP.csv";
      normalizeWrtOverallMax(in, out, ",");
      double mm = findOverallMax(out, ",");
      //System.out.println(mm);
   }

   public static void mmain(String[] args) throws Exception {
      final String in = "UtilityMatrix/data/141004/reduced/141003_oneOverNone_nowl_large_EXEC_TIME.csv";
      //int[] indices = new int[]{187, 213, 46, 96, 200, 11, 202, 21, 151, 146, 158, 18, 10, 217, 9};
      int[] indices = new int[]{179, 125, 70, 178, 207, 135, 82, 136, 150, 157, 158, 18, 10, 137, 9};
      for (int i : indices) {
         normalizeWrt(in, i, ",", "oneOverNone");
      }
   }

   public static void mmmain(String[] args) throws Exception {
      final String in = "UtilityMatrix/data/141004/reduced/141003_oneOverNone_nowl_large_EXEC_TIME.csv";
      final String out = "UtilityMatrix/data/141004/reduced/141003_LOGoneOverNone_nowl_large_EXEC_TIME.csv";
      logTransform(in, out, ",", 10);
   }

   public static void fsagfsagsagawgmain(String[] args) throws Exception {
      final String in = "TMParser/141114_epfl_none_nowl_large_EXEC_TIME.csv";
      final String out = "TMParser/141114_epfl_none_nowl_large_EXEC_TIME_norm.csv";
      normalizeWrtLowestPerRow(in, out);
   }


   public static void adfdasfmain(String[] args) throws IOException, ParameterNotFoundException {
      final String kpiFile = "UtilityMatrix/data/141029/bench/edp/141003_original_nowl_large_EDP_reducedColumnsNoNoise.csv";
      final String wlFile = "UtilityMatrix/data/141029/bench/workload/141118_wlFrom_141007_none_tinystm5_large.csv";
      final String outFile = "UtilityMatrix/data/141029/bench/workload/141003_original_withwl_large_EDP_reducedColumnsNoNoise.csv";
      MatrixHelper matrixHelper = new MatrixHelper(kpiFile, wlFile, outFile);
      matrixHelper.merge();
   }

   public static void _M_main(String[] args) throws Exception {
      final String reference = "UtilityMatrix/data/141029/bench/exec/141003_original_nowl_large_EXEC_TIME_reducedColumnsNoNoise.csv";
      final String in = "UtilityMatrix/data/141029/bench/edp/141003_original_nowl_large_EDP_reducedColumns.csv";
      final String out = "UtilityMatrix/data/141029/bench/edp/141003_original_nowl_large_EDP_reducedColumnsNoNoise.csv";
      copySameRow(reference, in, out);
   }

   public static void dfdsmain(String[] args) throws Exception {
      //final String in = "UtilityMatrix/data/141029/bench/exec/141003_original_nowl_large_EXEC_TIME_reducedColumnsNoNoise.csv";
      //final String out = "UtilityMatrix/data/141029/bench/exec/141003_originalNormalized_nowl_large_EXEC_TIME_reducedColumnsNoNoise.csv";
      final String in = "UtilityMatrix/data/141029/bench/epfl/141114_epfl_original_nowl_large_EXEC_TIME.csv";
      final String out = "UtilityMatrix/data/141029/bench/epfl/141114_epfl_originalNormalized_nowl_large_EXEC_TIME.csv";
      normalizeWrtLowestPerRow(in, out);
   }


   public static void main(String[] args) throws Exception {
      final String in =  "UtilityMatrix/data/sosp/EXEC/all.csv";// "UtilityMatrix/data/141029/bench/exec/141003_original_nowl_large_EXEC_TIME_reducedColumnsNoNoise.csv";
      final String out = "UtilityMatrix/data/sosp/EXEC/allNorm.csv";//"UtilityMatrix/data/150304/stamp_micro/OneOverStampNoRedBlackTree.csv";
      normalizeWrtHighestPerRow(in, out);
   }


}
