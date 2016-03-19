package utilityMatrix.recoEval.profiles;


import gnu.trove.map.TIntObjectMap;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

public class BasicUserProfileHolder<T> implements ProfilesHolder<T> {


   private final TIntObjectMap<T> mapProfiles_;


   public BasicUserProfileHolder(TIntObjectMap<T> profiles) {
      mapProfiles_ = profiles;
   }

   @Override
   public T getProfile(int user) {
      return mapProfiles_.get(user);

   }

   @Override
   public int getNbUsers() {
      return mapProfiles_.size();
   }

   public int[] getUserIds() {
      return mapProfiles_.keys();
   }

   @Override
   public void dumpToFile(String file) {
      File f = new File("../output/" + file);
      try {
         PrintWriter pw = new PrintWriter(new FileWriter(f));
         if (mapProfiles_ != null) {
            for (T t : mapProfiles_.valueCollection()) {
               System.out.println(t.toString());
            }
         } else {
            throw new RuntimeException("Map is null");
         }
      } catch (IOException e) {
         throw new RuntimeException(e);
      }
   }

   //Added by Diego. These will only work with the Map: I can't see any active portion of code
   //Which is using the array

   @Override
   public boolean addIfAbsent(int id, T u) {
      if (mapProfiles_.containsKey(id)) {
         /*if (id == 0) {
            System.out.println(id + " already here!! size "+mapProfiles_.size()+":" + mapProfiles_.keySet());
            System.out.println("its value is " + mapProfiles_.get(id));
         }*/
         return false;
      }
      /*if (id == 0) {
         System.out.println("Adding user " + id);
      }*/
      mapProfiles_.put(id, u);
      return true;
   }

   @Override
   public boolean addIfAbsentOrEmpty(int id, T u) {
      //Add if absent
      boolean added = addIfAbsent(id, u);
      //If now absent (remember that the train/test splitters keep profiles even for the empty users!
      if (!added) {
         //check whether it is empty
         T t = mapProfiles_.get(id);
         UserProfile up = (UserProfile) t;
         if (up.getNbItems() == 0 && ((BasicUserProfile) up).isEmpty()) {
            boolean removed = this.removeIfPresent(id) == null;
            if (!removed) {
               throw new RuntimeException("This should have been removed by now!");
            }
            added = this.addIfAbsent(id, u);
            if (!added) {
               throw new RuntimeException("This should have been added by now!");
            }
         } else {
            BasicUserProfile b = (BasicUserProfile) u;
            b.dump();
            System.out.println("User " + u + " has " + up.getNbItems() + " items!");
         }
      }
      return added;
   }

   @Override
   public T removeIfPresent(int id) {
      if (!mapProfiles_.containsKey(id)) {
         System.out.println("Impossible to remove workload with id " + id + "\n" + mapProfiles_.toString());
         return null;
      }
      int preSize = mapProfiles_.size();
      T t = mapProfiles_.remove(id);
      /*if (id == 0) {
         System.out.println("Pre " + preSize + " post " + mapProfiles_.size() + " Removed " + id + "==> " + t);
      }*/
      return t;
   }

   @Override
   public String toString() {
      return "BasicUserProfileHolder{" +
            "mapProfiles_=" + mapProfiles_ +
            '}';
   }
}
