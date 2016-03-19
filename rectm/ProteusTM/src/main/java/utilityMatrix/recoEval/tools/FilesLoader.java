package utilityMatrix.recoEval.tools;

import java.io.BufferedWriter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class FilesLoader {

   @SuppressWarnings("unchecked")
   public static <T> T loadGeneric(String f) throws IOException, ClassNotFoundException {
      FileInputStream fos = new FileInputStream(f);
      ObjectInputStream oos = new ObjectInputStream(fos);
      T p = (T) oos.readObject();
      fos.close();
      return p;
   }

   public static void logString(String filename, String str) {
      logString(filename, str, false);
   }

   public static void logString(String filename, String str, boolean append) {
      try {
         // Create non-existing directories in the path represented by
         // filename
         Path dirname = Paths.get(filename).getParent();
         Files.createDirectories(dirname);
         // Create file
         FileWriter fstream = new FileWriter(filename, append);
         BufferedWriter out = new BufferedWriter(fstream);
         out.write(str);
         out.close();
      } catch (Exception e) {
         throw new RuntimeException(e);
      }
   }

   public static void saveWhatever(String filename, Object o) {
      try {
         FileOutputStream fos = new FileOutputStream(filename);
         ObjectOutputStream oos = new ObjectOutputStream(fos);

         oos.writeObject(o);
         oos.flush();
         fos.close();

      } catch (IOException e1) {
         e1.printStackTrace();
      }

      System.out.println("Saved Whatever in " + filename);
   }


   // Reflexion

   @SuppressWarnings("unchecked")
   public static <T> T getField(Object target, String field) {
      if (target == null) {
         throw new RuntimeException("Trying to get a field on a null object : " + field);
      }
      try {
         Class<?> classe = target.getClass();
         while (!classe.equals(Object.class)) {
            Field[] fields = classe.getDeclaredFields();
            for (Field f : fields) {
               if (f.getName().equals(field)) {
                  if (!f.isAccessible()) {
                     f.setAccessible(true);
                  }
                  return (T) f.get(target);
               }
            }
            classe = classe.getSuperclass();
         }
      } catch (Exception e) {
         throw new RuntimeException(e.getMessage());
      }
      throw new RuntimeException("Could not retrieve the field : " + field + " in " + target.getClass());
   }

   public static void setField(Object target, String field, Object value) {
      if (target == null) {
         System.err.println("Trying to get a field on a null object : " + field);
         return;
      }
      try {
         Class<?> classe = target.getClass();
         while (!classe.equals(Object.class)) {
            // System.out.println("Searching " + field + " in " +
            // classe.getName());
            Field[] fields = classe.getDeclaredFields();
            for (Field f : fields) {
               if (f.getName().equals(field)) {
                  f.setAccessible(true);
                  // System.out.println("Field found");
                  f.set(target, value);
               }
            }
            // System.out.println("Not found in " + classe.getName());
            classe = classe.getSuperclass();
         }
         // System.out.println("Field Not found in " + target.getClass());
      } catch (Exception e) {
         System.out.println("Could not set the field : " + target.getClass());
         e.printStackTrace();
      }
   }
}
