package utilityMatrix.recoEval.tools;

import gnu.trove.map.TIntObjectMap;
import gnu.trove.map.hash.TIntObjectHashMap;

public class SampleSize {

   private static final TIntObjectMap<int[]> samples_ = new TIntObjectHashMap<int[]>();
   private final static double multiplicator = 1.5;

   /**
    * Creates different sample sizes, i.e., number of neighbors that will be used to perform recommendations
    *
    * @param nbNodes
    * @return
    */
   public static int[] getSamplesSizes(int nbNodes) {
      nbNodes = Math.min(nbNodes, 10000);
      int[] samples = samples_.get(nbNodes);
      if (samples == null) {
         int nbSamples = 1;
         for (int sample = 2; sample < nbNodes - 1; sample *= multiplicator) {
            nbSamples++;
         }

         samples = new int[nbSamples];
         int index = 0;
         for (int sample = 2; sample < nbNodes - 1; sample *= multiplicator) {
            samples[index++] = sample;
         }
         samples[index++] = nbNodes - 1;
         samples_.put(nbNodes, samples);
      }
      return samples;
   }
}
