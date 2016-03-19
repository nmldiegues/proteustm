package utilityMatrix.recoEval.datasetLoader;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 25/07/14
 */
public class ProbabilisticCSVLoader extends CSVLoaderSplit {

   private double fractionTraining;

   public ProbabilisticCSVLoader(String file, double fractionTraining, boolean training, long seed) {
      super(file, training, seed);
      this.fractionTraining = fractionTraining;
   }

   public ProbabilisticCSVLoader(String file, double fractionTraining, boolean training) {
      super(file, training);
      this.fractionTraining = fractionTraining;
   }

   @Override
   protected boolean copyCell(int rowIndex, int columnIndex) {
      return
            isTraining && random.nextDouble() < fractionTraining ||
                  !isTraining && random.nextDouble() > fractionTraining;
   }

   @Override
   protected void _newLine(int line) {
      //nop
   }
}
