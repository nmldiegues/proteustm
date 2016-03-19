package utilityMatrix.recoEval.tools;

import org.apache.commons.io.FileUtils;
import utilityMatrix.RecommenderConfig;
import utilityMatrix.mahout.knn.KNNConfig;
import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeSet;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 27/07/14
 */
public class Averager {

   private final double numRuns;
   private final String outputFolder;
   private final boolean _DEL_ON_AVG_ = true;
   private final String overall = "overall";
   private final String matrix = "matrix";
   private final PrintWriter ovl;
   private final String resultFolder;
   private final String id;
   private ExtendedParameters extendedParameters;
   private RecommenderConfig recommenderConfig;

   public Averager(double numRuns, String outputF, String resultF, String id, ExtendedParameters ep, RecommenderConfig recommenderConfig) {
      this.numRuns = numRuns;
      this.outputFolder = outputF;
      this.resultFolder = resultF;
      this.id = id;
      this.extendedParameters = ep;
      this.recommenderConfig = recommenderConfig;
      try {
         ovl = new PrintWriter(new FileWriter(new File(finalOutputFileName())));
         ovl.println("ROW,COLUMN,NUM_REC," + wPrefix("FAILED_REC,") + wPrefix("MAE,") + wPrefix("RMSE,") + wPrefix("MAPE,") + wPrefix("CORR,") + wPrefix("RSQ,") + wPrefix("ERROR_RATE,") + wPrefix("AVG_DIST_FROM_OPT_GIVEN_WRONG,") + wPrefix("AVG_DIST_FROM_OPT,") + wPrefix("AVG_DIST_BY_TRAIN,") + wPrefix("MAPE_BY_ROW,") + wPrefix("MAPE_BY_COL,") + "MAPE_2,RMSE2,MAX_DIST,90_DIST,MEAN_DIST,MED_DIST,MAX_DIST_TRAIN,90_DIST_TRAIN,MEAN_DIST_TRAIN,MED_DIST_TRAIN,NN");
      } catch (IOException e) {
         throw new RuntimeException(e);
      }
   }

   private String finalOutputFileName() {
      return resultFolder + "/" + id + ".data";
   }

   private boolean doAvg(String name) {
      return name.contains("average") &&
            !name.contains("Random") && !name.endsWith(".avg");
   }

   private boolean isToDelete(String name) {
      return _DEL_ON_AVG_ &&
            !name.contains(overall) &&
            !name.contains(matrix);
   }

   private String wPrefix(String i) {
      return prefix() + i;
   }

   private boolean moveFileIfAvg(String name, String toName) {
      if (name.endsWith("average.dat")) {
         String[] split = name.split("-");
         //Result--2-2-average.dat
         int size = split.length;
         String append = split[size - 2] + "_" + split[size - 3];
         String finalToName = toName.replace(".data", append + ".all");
         try {
            FileUtils.copyFile(new File(name), new File(finalToName));
            System.out.println(name + " moved to " + toName);
         } catch (IOException e) {
            e.printStackTrace();
            System.err.println("It was impossible to move " + name + " to " + (finalToName));
         }
         return true;
      }
      return false;
   }

   public void avgResults() throws IOException {
      final File[] results = new File(outputFolder).listFiles();
      assert results != null;
      String name;
      for (File avg : results) {
         name = avg.getName();
         boolean moved = true;
         if (doAvg(name)) {
            String outname = outputFolder + "/" + name.concat(".avg");
            String prefix = extractTrainRows(name) + "," + extractFixedColumns(name);
            avgFile(avg, outname, prefix);
            moved = (moveFileIfAvg(avg.getAbsolutePath(), (resultFolder + "/" + id)));
         }
         if (!moved && isToDelete(name)) {
            avg.delete();
         }
      }
      ovl.close();
   }

   /**
    * XXXX-Y-average.data
    *
    * @param name
    * @return
    */
   private int extractFixedColumns(String name) {
      String[] a = name.split("-");
      return Integer.parseInt(a[a.length - 2]);
   }

   private int extractTrainRows(String name) {
      String[] a = name.split("-");
      return Integer.parseInt(a[a.length - 3]);
   }

   private void avgFile(File srcFile, String avgOut, String prefix) throws IOException {
      HashMap<Double, Double[]> idToArray = new HashMap<>();
      BufferedReader br = new BufferedReader(new FileReader(srcFile));
      String read;
      System.out.println("Parsing " + srcFile);
      while ((read = br.readLine()) != null) {
         Double[] parsed = parseLine(read);
         add(idToArray, parsed);
      }
      avg(idToArray);
      PrintWriter pw = new PrintWriter(new FileWriter(new File(avgOut)));
      TreeSet<Double> ordered = new TreeSet<>();
      for (Double d : idToArray.keySet()) {
         ordered.add(d);
      }
      Iterator<Double> it = ordered.iterator();
      while (it.hasNext()) {
         Double[] ee = idToArray.get(it.next());
         pw.print(prefix);
         ovl.print(prefix);
         pw.print(",");
         ovl.print(",");
         for (Double d : ee) {
            pw.print(d + ",");
            ovl.print(d + ",");
         }
         pw.print(NN());
         ovl.print(NN());
         pw.print("\n");
         ovl.print("\n");
      }
      pw.close();
   }

   int z = 0;

   private void add(HashMap<Double, Double[]> map, Double[] doubles) {
      double id = doubles[0];
      Double[] current = map.get(id);
      if (current == null) {
         //System.out.println((++z) + "Creating " + id + " " + Arrays.toString(doubles));
         map.put(id, doubles);
      } else {
         for (int i = 0; i < current.length; i++) {
            current[i] += doubles[i];
         }
         //System.out.println((++z) + " Updating " + id + " " + Arrays.toString(current));
      }
   }

   private void avg(HashMap<Double, Double[]> map) {
      for (Map.Entry<Double, Double[]> entry : map.entrySet()) {
         Double[] value = entry.getValue();
         double l = value.length;
         for (int i = 0; i < l; i++) {
            value[i] = value[i] / numRuns;
         }
      }
   }

   private Double[] parseLine(String line) {
      //System.out.println(line);
      final String[] split = line.split("\\s+");
      //final String[] split = line.split(",");
      final Double[] ret = new Double[split.length];
      for (int i = 0; i < split.length; i++) {
         ret[i] = Double.parseDouble(split[i]);
      }
      return ret;
   }


   private int NN() {
      if (!(recommenderConfig instanceof KNNConfig)) {
         return -1;
      }
      return ((KNNConfig) recommenderConfig).getNumNeighbors();
   }

   private String userItemNN() {
      if (!(recommenderConfig instanceof KNNConfig)) {
         return "";
      }
      return ((KNNConfig) recommenderConfig).getUserItemSimilarity().toString();
   }

   private String prefix() {
      if (true)
         return "";
      String reco = extendedParameters.getRecmode_();
      Normalizator.normalization normalization = this.extendedParameters.getDataNormalization();
      String wrt = normalization.equals(Normalizator.normalization.WRT_REF) ? "_" + extendedParameters.getNormalizeWRT() : "";
      return reco + "_" + normalization.toString() + wrt;
   }
}
