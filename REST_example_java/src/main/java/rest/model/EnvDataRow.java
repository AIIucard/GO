package rest.model;

import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
public class EnvDataRow {
	
	private float temperature;
	private float humidity;
	
	public EnvDataRow(){
		
	}

	public EnvDataRow(float temperature, float humidity) {
		super();
		this.temperature = temperature;
		this.humidity = humidity;
	}

	public float getTemperature() {
		return temperature;
	}

	public void setTemperature(float temperature) {
		this.temperature = temperature;
	}

	public float getHumidity() {
		return humidity;
	}

	public void setHumidity(float humidity) {
		this.humidity = humidity;
	}

	@Override
	public String toString() {
		return "EnvDataRow [temperature=" + temperature + ", humidity=" + humidity + "]";
	}

}
