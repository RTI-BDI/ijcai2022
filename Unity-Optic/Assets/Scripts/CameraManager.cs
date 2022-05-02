using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraManager : MonoBehaviour
{

	private Vector3 ResetCamera;
	

	public float zoomSpeed = 2;
	public float targetOrtho;
	public float smoothSpeed = 2.0f;
	public float minOrtho = 2.0f;
	public float maxOrtho = 10.0f;

	// Start is called before the first frame update
	void Start()
    {
		ResetCamera = Camera.main.transform.position;
		targetOrtho = Camera.main.orthographicSize;
	}

    // Update is called once per frame
    void LateUpdate()
    {
		float xAxisValue = Input.GetAxis("Horizontal");
		float yAxisValue = Input.GetAxis("Vertical");

		transform.position = new Vector3(transform.position.x + xAxisValue, transform.position.y + yAxisValue, transform.position.z);

		//RESET CAMERA TO STARTING POSITION WITH RIGHT CLICK
		if (Input.GetMouseButton(1))
		{
			Collider2D hitCollider = Physics2D.OverlapPoint(Camera.main.ScreenToWorldPoint(Input.mousePosition));
			if (hitCollider == null)
			{
				Camera.main.transform.position = ResetCamera;
			}
		}

		float scroll = Input.GetAxis("Mouse ScrollWheel");
		if (scroll != 0.0f)
		{
			targetOrtho -= scroll * zoomSpeed;
			targetOrtho = Mathf.Clamp(targetOrtho, minOrtho, maxOrtho);
		}

		Camera.main.orthographicSize = Mathf.MoveTowards(Camera.main.orthographicSize, targetOrtho, smoothSpeed * Time.deltaTime);

	}
}
