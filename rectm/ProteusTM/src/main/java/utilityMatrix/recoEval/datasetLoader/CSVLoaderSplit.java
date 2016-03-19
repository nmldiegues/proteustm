package utilityMatrix.recoEval.datasetLoader;

import gnu.trove.iterator.TIntObjectIterator;
import gnu.trove.map.TIntDoubleMap;
import gnu.trove.map.TIntObjectMap;
import gnu.trove.map.hash.TIntDoubleHashMap;
import gnu.trove.map.hash.TIntObjectHashMap;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;
import utilityMatrix.recoEval.profiles.BasicUserProfile;
import utilityMatrix.recoEval.profiles.BasicUserProfileHolder;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Random;

/**
 * Created by ajegou on 22/07/14.
 */
public abstract class CSVLoaderSplit implements DatasetLoader {

   protected final static String SEPARATOR = ",";
   private ProfilesHolder<UserProfile> profiles_;

   private int nbItems_;
   private final static long DEF_SEED = 654343543;

   protected final boolean isTraining;
   protected final String srcFile;
   protected final Random random;
   protected final static Log log = LogFactory.getLog(CSVLoaderSplit.class);
   protected final static boolean trace = log.isTraceEnabled();
   protected final static boolean debug = log.isDebugEnabled();
   protected final long fixedSeed;

   private Normalizator normalizator;

   public void setNormalizator(Normalizator constantNormalizator) {
      this.normalizator = constantNormalizator;
   }

   public CSVLoaderSplit(String file, boolean training, long seed) {
      this.srcFile = file;
      this.isTraining = training;
      this.fixedSeed = seed;
      this.random = new Random(seed);
   }

   public CSVLoaderSplit(String file, boolean training) {
      this(file, training, DEF_SEED);
   }

   protected int initialUserId() {
      return 0;
   }

   public void split() throws IOException {
      if (this.normalizator == null) {
         throw new IllegalStateException("Normalizator should be non-null from now on");
      }

      TIntObjectMap<UserProfile> profiles = new TIntObjectHashMap<>();

      BufferedReader reader = new BufferedReader(new FileReader(this.srcFile));

      //Ignore the first row containing columns names, i.e., the header,
      String line = reader.readLine();

      int nbLoadedentries = 0;

      int nbItems = -1;

      int userId = initialUserId();    //UserId will be 0-based (normally)
      while ((line = reader.readLine()) != null) {
         String[] split = line.split(SEPARATOR);
         TIntDoubleMap profile = new TIntDoubleHashMap();
         _newLine(userId);
        // if (isTraining && log.isInfoEnabled()) log.info("\nSPLITTING " + userId);
         // Ignore the row name, i.e., first column: items will be 1-based
         for (int conf = 1; conf < split.length; conf++) {
            double score;

            try {
               score = Double.parseDouble(split[conf]);
            } catch (NumberFormatException n) {
               System.out.println("Problem at cell " + userId + "," + conf + ". Parsing 0");
               score = 0;
            }
            /**
             * Use the conf-th element in the appId-th profile with a given probability
             */
            if (copyCell(userId, conf)) {
               //if (isTraining && log.isInfoEnabled()) log.info("(" + userId + ", " + conf + ") ");
               if (normalizator != null) {
                  score = normalizator.normalize(userId, conf, score);
               }
               profile.put(conf, score);
               nbLoadedentries++;
            }
         }
         profiles.put(userId, new BasicUserProfile(profile));
         userId++;

         if (nbItems == -1) {
            nbItems = split.length;
         } else {
            if (nbItems != split.length) {
               System.err.println("Different lines have a different number of columns: " + nbItems + " " + split.length);
            }
         }
      }

      //System.out.println("Loaded " + nbLoadedentries + " entries.");
      TIntObjectIterator<UserProfile> it = profiles.iterator();
      if (log.isTraceEnabled()) {
         if (isTraining) {
            log.trace("TRAIN");
         } else {
            log.trace("TEST");
         }
         while (it.hasNext()) {
            it.advance();
            log.trace(it.key() + " ==>" + it.value());
         }
      }
      profiles_ = new BasicUserProfileHolder<UserProfile>(profiles);
      nbItems_ = nbItems;
   }


   protected abstract boolean copyCell(int rowIndex, int columnIndex);


   @Override
   public ProfilesHolder<UserProfile> getUserProfiles() {
      return profiles_;
   }

   @Override
   public int getNbUsers() {
      return profiles_.getNbUsers();
   }

   @Override
   public int getNbDimensions() {
      return nbItems_;
   }

   @Override
   public String getName() {
      return "CVSLoader";
   }

   protected abstract void _newLine(int rowIndex);
}
