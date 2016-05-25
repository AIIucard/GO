package rest.main;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;

import sun.util.logging.PlatformLogger.Level;

public class Client {

	public static void main(String[] args) throws Exception {
		sun.util.logging.PlatformLogger.getLogger("sun.net.www.protocol.http.HttpURLConnection").setLevel(Level.ALL);
		
		doGet();
		doPost();
		doGet();
	}
	
	public static void doGet() throws Exception {
		URL url = new URL("http://localhost:5000/rest/environmentData");
		HttpURLConnection conn = (HttpURLConnection) url.openConnection();
		conn.setRequestMethod("GET");
		conn.setRequestProperty("Accept", "application/json");

		if (conn.getResponseCode() != 200) {
			throw new RuntimeException("Failed : HTTP error code : " + conn.getResponseCode());
		}

		BufferedReader br = new BufferedReader(new InputStreamReader((conn.getInputStream())));

		String output;
		System.out.println("Output from Server ....");
		while ((output = br.readLine()) != null) {
			System.out.println(output);
		}
		System.out.println();

		conn.disconnect();
	}

	public static void doPost() throws Exception {
		HttpURLConnection con = (HttpURLConnection) new URL("http://localhost:5000/rest/environmentData")
				.openConnection();
		con.setRequestProperty("Content-Type", "application/json");
		con.setRequestMethod("POST");
		con.setDoOutput(true);
		OutputStreamWriter out = new OutputStreamWriter(con.getOutputStream());
		out.write("{\"temperature\" : 24.2, \"humidity\" : 69.23}");

		out.close();

		System.out.println(con.getResponseCode());

		con.disconnect();
	}

}
