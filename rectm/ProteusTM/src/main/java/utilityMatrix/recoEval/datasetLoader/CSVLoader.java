package utilityMatrix.recoEval.datasetLoader;

import gnu.trove.map.TIntDoubleMap;
import gnu.trove.map.TIntObjectMap;
import gnu.trove.map.hash.TIntDoubleHashMap;
import gnu.trove.map.hash.TIntObjectHashMap;
import utilityMatrix.recoEval.profiles.BasicUserProfile;
import utilityMatrix.recoEval.profiles.BasicUserProfileHolder;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

/**
 * Created by ajegou on 22/07/14.
 */
public class CSVLoader implements DatasetLoader {

   private final static String SEPARATOR = ",";
   private final ProfilesHolder<UserProfile> profiles_;

   private final int nbItems_;

   public CSVLoader(String file) throws IOException {

      TIntObjectMap<UserProfile> profiles = new TIntObjectHashMap<>();

      BufferedReader reader = new BufferedReader(new FileReader(file));

      String line;

      int nbItems = -1;

      int appId = 0;
      while ((line = reader.readLine()) != null) {
         String[] split = line.split(SEPARATOR);
         TIntDoubleMap profile = new TIntDoubleHashMap();
         for (int conf = 0; conf < split.length; conf++) {
            double score = Double.parseDouble(split[conf]);
            if (score >= 0) {
               profile.put(conf, score);
            }
         }
         profiles.put(appId, new BasicUserProfile(profile));
         appId++;

         if (nbItems == -1) {
            nbItems = split.length;
         } else {
            if (nbItems != split.length) {
               System.err.println("Different lines have a different number of columns: " + nbItems + " " + split.length);
            }
         }
      }


      profiles_ = new BasicUserProfileHolder<UserProfile>(profiles);
      nbItems_ = nbItems;
   }

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


   public static void prettyPrint(ProfilesHolder<UserProfile> profiles, int numColumns) {
      int full = 0;
      for (int i : profiles.getUserIds()) {
         System.out.println(i + " == " + profiles.getProfile(i));
         if (isFullyProfiled(numColumns, profiles.getProfile(i)))
            full++;
      }
      System.out.println("Fully profiled: " + full);
   }

   private static boolean isFullyProfiled(int numElements, UserProfile up) {
      for (int i = 1; i <= numElements; i++) {
         if (!up.contains(i))
            return false;
      }
      return true;
   }

}
